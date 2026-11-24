// Keyboard, mouse and gamepads for the key map, on macOS. A thread of its own runs a CFRunLoop: tasks posted to
// the capture run there, and the handler is woken when it asks (nextWakeup), as on Windows.
//
// The devices themselves are not read yet on macOS: started() is false and problem() says so.
#include "gs/platform/InputCapture.h"

#include "gs/base/Clock.h"
#include "gs/platform/Thread.h"

#include <CoreFoundation/CoreFoundation.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

namespace gs::platform {

struct InputCapture::Impl {
    InputHandler& handler;
    std::mutex mutex;
    std::condition_variable ready;
    CFRunLoopRef loop = nullptr;
    CFRunLoopTimerRef timer = nullptr;
    std::deque<std::function<void()>> tasks;
    bool stopping = false;
    std::uintptr_t captureTarget = 0;
    std::thread thread;

    explicit Impl(InputHandler& given) : handler(given) {}

    void armWake()
    {
        const std::optional<Microseconds> next = handler.nextWakeup();
        if (!next) {
            CFRunLoopTimerSetNextFireDate(timer, CFAbsoluteTimeGetCurrent() + 3600.0 * 24 * 365);
            return;
        }
        const double seconds = std::chrono::duration<double>(*next - InputCapture::now()).count();
        CFRunLoopTimerSetNextFireDate(timer, CFAbsoluteTimeGetCurrent() + std::max(seconds, 0.0));
    }

    void runTasks()
    {
        std::deque<std::function<void()>> taken;
        {
            const std::scoped_lock lock(mutex);
            taken.swap(tasks);
        }
        for (std::function<void()>& task : taken) {
            task();
        }
        armWake();
    }

    void run()
    {
        setCurrentThreadName("gs-input");
        CFRunLoopTimerContext context{0, this, nullptr, nullptr, nullptr};
        timer = CFRunLoopTimerCreate(
            nullptr, CFAbsoluteTimeGetCurrent() + 3600.0, 3600.0 * 24 * 365, 0, 0,
            [](CFRunLoopTimerRef, void* info) {
                auto* self = static_cast<Impl*>(info);
                self->handler.wake(InputCapture::now());
                self->armWake();
            },
            &context);
        CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
        {
            const std::scoped_lock lock(mutex);
            loop = CFRunLoopGetCurrent();
            CFRetain(loop);
        }
        ready.notify_all();
        runTasks();
        while (true) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, false);
            const std::scoped_lock lock(mutex);
            if (stopping) {
                break;
            }
        }
        CFRunLoopTimerInvalidate(timer);
        CFRelease(timer);
    }

    void post(std::function<void()> task)
    {
        CFRunLoopRef target = nullptr;
        {
            const std::scoped_lock lock(mutex);
            tasks.push_back(std::move(task));
            target = loop;
        }
        if (target != nullptr) {
            CFRunLoopPerformBlock(target, kCFRunLoopCommonModes, ^{
                runTasks();
            });
            CFRunLoopWakeUp(target);
        }
    }
};

InputCapture::InputCapture(InputHandler& handler, Microseconds) : m_impl(std::make_unique<Impl>(handler))
{
    Impl* impl = m_impl.get();
    impl->thread = std::thread([impl] { impl->run(); });
    std::unique_lock lock(impl->mutex);
    impl->ready.wait(lock, [impl] { return impl->loop != nullptr; });
}

InputCapture::~InputCapture()
{
    CFRunLoopRef loop = nullptr;
    {
        const std::scoped_lock lock(m_impl->mutex);
        m_impl->stopping = true;
        loop = m_impl->loop;
    }
    CFRunLoopStop(loop);
    CFRunLoopWakeUp(loop);
    m_impl->thread.join();
    CFRelease(loop);
}

bool InputCapture::started() const noexcept
{
    return false;
}

std::string InputCapture::problem() const
{
    return "the keyboard, mouse and gamepads are not read on macOS yet";
}

Microseconds InputCapture::now() noexcept
{
    return SteadyClock{}.now();
}

void InputCapture::post(std::function<void()> task)
{
    m_impl->post(std::move(task));
}

void InputCapture::setCaptureTarget(std::uintptr_t window)
{
    post([impl = m_impl.get(), window] { impl->captureTarget = window; });
}

void InputCapture::rumble(std::uint16_t, std::uint16_t, std::uint32_t) {}

void InputCapture::rumbleTriggers(std::uint16_t, std::uint16_t, std::uint32_t) {}

}
