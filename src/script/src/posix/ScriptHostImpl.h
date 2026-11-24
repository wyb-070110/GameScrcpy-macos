// The script thread off Windows: a pthread with the stack the engine asks for, woken by a condition variable
// for commands and by its deadline for timers and frames. Included by ScriptHost.cpp inside gs::script.

namespace {

constexpr std::size_t kStackMargin = std::size_t{1} << 20;

}

struct ScriptHost::Impl {
    using Command = std::function<void(detail::Engine&)>;

    Impl(Sink target, const Limits& configured, std::shared_ptr<ScriptServices> provided)
        : sink(std::move(target)), limits(configured), services(std::move(provided))
    {
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    static void* entry(void* opaque);
    void run();
    void post(Command command);
    void runCommands(detail::Engine& engine);
    void arm(std::optional<Microseconds> due, Microseconds now);
    void wait();
    void fail(std::string what);

    Sink sink;
    const Limits limits;
    std::shared_ptr<ScriptServices> services;
    pthread_t thread{};
    bool threadStarted = false;
    std::atomic<bool> stopRequested{false};
    std::atomic<ScriptId> lastScript{0};
    std::atomic<TaskId> lastTask{0};
    mutable std::mutex mutex;
    std::condition_variable woken;
    bool wakePending = false;
    std::deque<Command> commands;
    std::string problem;
    std::optional<Microseconds> armedFor;
    std::optional<std::chrono::steady_clock::time_point> deadline;
    Microseconds frameInterval{0};
    std::optional<Microseconds> nextFrame;
};

void* ScriptHost::Impl::entry(void* opaque)
{
    static_cast<Impl*>(opaque)->run();
    return nullptr;
}

void ScriptHost::Impl::run()
{
    platform::setCurrentThreadName("gs-script");
    platform::setCurrentThreadRole(platform::ThreadRole::Input);
    detail::Engine engine(sink, limits, stopRequested, lastTask, detail::EngineOptions{.services = services});
    while (!stopRequested.load(std::memory_order_acquire)) {
        runCommands(engine);
        if (stopRequested.load(std::memory_order_acquire)) {
            break;
        }
        try {
            engine.fireTimers();
            if (nextFrame && engine.now() >= *nextFrame) {
                nextFrame.reset();
                engine.frame();
            }
        } catch (...) {
            fail("a script timer failed");
        }
        std::optional<Microseconds> due = engine.nextTimer();
        if (frameInterval.count() > 0 && engine.waitingForFrame()) {
            if (!nextFrame) {
                nextFrame = engine.now() + frameInterval;
            }
            due = due ? std::min(*due, *nextFrame) : *nextFrame;
        } else {
            nextFrame.reset();
        }
        arm(due, engine.now());
        wait();
    }
    try {
        engine.shutdown();
    } catch (...) {
        fail("the scripts could not be stopped cleanly");
    }
}

void ScriptHost::Impl::post(Command command)
{
    {
        const std::scoped_lock lock(mutex);
        commands.push_back(std::move(command));
        wakePending = true;
    }
    woken.notify_one();
}

void ScriptHost::Impl::runCommands(detail::Engine& engine)
{
    std::deque<Command> pending;
    {
        const std::scoped_lock lock(mutex);
        pending.swap(commands);
    }
    for (Command& command : pending) {
        if (stopRequested.load(std::memory_order_acquire)) {
            return;
        }
        try {
            command(engine);
        } catch (...) {
            fail("a script command failed");
        }
    }
}

void ScriptHost::Impl::arm(std::optional<Microseconds> due, Microseconds now)
{
    if (due == armedFor) {
        return;
    }
    armedFor = due;
    if (!due) {
        deadline.reset();
        return;
    }
    deadline = std::chrono::steady_clock::now() + std::max(Microseconds{1}, *due - now);
}

void ScriptHost::Impl::wait()
{
    std::unique_lock lock(mutex);
    const auto ready = [this] { return wakePending || stopRequested.load(std::memory_order_acquire); };
    if (deadline) {
        if (!woken.wait_until(lock, *deadline, ready)) {
            armedFor.reset();
            deadline.reset();
        }
    } else {
        woken.wait(lock, ready);
    }
    wakePending = false;
}

void ScriptHost::Impl::fail(std::string what)
{
    const std::scoped_lock lock(mutex);
    if (problem.empty()) {
        problem = std::move(what);
    }
}

ScriptHost::ScriptHost(Sink sink, Limits limits, std::shared_ptr<ScriptServices> services)
    : m_impl(std::make_unique<Impl>(std::move(sink), limits, std::move(services)))
{
    pthread_attr_t attributes;
    if (pthread_attr_init(&attributes) != 0) {
        m_impl->fail("could not start the script thread");
        return;
    }
    const long page = sysconf(_SC_PAGESIZE);
    const std::size_t pageSize = page > 0 ? static_cast<std::size_t>(page) : 4096;
    std::size_t stack = limits.stackBytes + kStackMargin;
    stack = std::max<std::size_t>((stack + pageSize - 1) / pageSize * pageSize, PTHREAD_STACK_MIN);
    pthread_attr_setstacksize(&attributes, stack);
    m_impl->threadStarted = pthread_create(&m_impl->thread, &attributes, &Impl::entry, m_impl.get()) == 0;
    pthread_attr_destroy(&attributes);
    if (!m_impl->threadStarted) {
        m_impl->fail("could not start the script thread");
    }
}

ScriptHost::~ScriptHost()
{
    {
        const std::scoped_lock lock(m_impl->mutex);
        m_impl->stopRequested.store(true, std::memory_order_release);
        m_impl->wakePending = true;
    }
    m_impl->woken.notify_one();
    if (m_impl->threadStarted) {
        pthread_join(m_impl->thread, nullptr);
        m_impl->threadStarted = false;
    }
}

bool ScriptHost::started() const noexcept
{
    return m_impl->threadStarted;
}
