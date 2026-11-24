// The system's voices on macOS: AVSpeechSynthesizer speaks a line with a chosen voice, rate and volume. Voice
// commands (listen) and rendering speech to PCM (record) are not on macOS yet; listen says so through stopped.
#include "gs/platform/Speech.h"

#import <AVFoundation/AVFoundation.h>

#include <map>
#include <mutex>

namespace gs::platform {

struct SystemSpeech::Impl {
    std::mutex mutex;
    std::uint64_t next = 1;
    std::map<std::uint64_t, Spoken> spoken;
    std::map<std::uint64_t, AVSpeechSynthesizer*> speakers;
    id delegate = nil;
    bool silent = false;
};

void speechFinished(void* owner, void* synthesizer, bool finished)
{
    if (owner == nullptr) {
        return;
    }
    SystemSpeech::Impl& impl = *static_cast<SystemSpeech*>(owner)->m_impl;
    SystemSpeech::Spoken done;
    {
        const std::scoped_lock lock(impl.mutex);
        for (auto it = impl.speakers.begin(); it != impl.speakers.end(); ++it) {
            if ((__bridge void*)it->second == synthesizer) {
                const auto found = impl.spoken.find(it->first);
                if (found != impl.spoken.end()) {
                    done = std::move(found->second);
                    impl.spoken.erase(found);
                }
                impl.speakers.erase(it);
                break;
            }
        }
    }
    if (done) {
        done(finished, finished ? std::string() : std::string("stopped"));
    }
}

}

@interface GSSpeechDelegate : NSObject <AVSpeechSynthesizerDelegate>
@property(nonatomic, assign) void* owner;
@end

@implementation GSSpeechDelegate
- (void)finish:(AVSpeechSynthesizer*)synthesizer finished:(bool)finished
{
    gs::platform::speechFinished(self.owner, (__bridge void*)synthesizer, finished);
}
- (void)speechSynthesizer:(AVSpeechSynthesizer*)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance*)utterance
{
    [self finish:synthesizer finished:true];
}
- (void)speechSynthesizer:(AVSpeechSynthesizer*)synthesizer didCancelSpeechUtterance:(AVSpeechUtterance*)utterance
{
    [self finish:synthesizer finished:false];
}
@end

namespace gs::platform {

namespace {

std::string utf8(NSString* text)
{
    return text != nil ? std::string([text UTF8String]) : std::string();
}

NSString* nsText(const std::string& text)
{
    NSString* made = [NSString stringWithUTF8String:text.c_str()];
    return made != nil ? made : @"";
}

}

SystemSpeech::SystemSpeech(SpeechOutput output) : m_impl(std::make_unique<Impl>())
{
    m_impl->silent = output == SpeechOutput::Nowhere;
    GSSpeechDelegate* delegate = [[GSSpeechDelegate alloc] init];
    delegate.owner = this;
    m_impl->delegate = delegate;
}

SystemSpeech::~SystemSpeech()
{
    const std::scoped_lock lock(m_impl->mutex);
    for (auto& [id, speaker] : m_impl->speakers) {
        speaker.delegate = nil;
        [speaker stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
    }
}

std::vector<SpeechVoice> SystemSpeech::voices() const
{
    std::vector<SpeechVoice> found;
    @autoreleasepool {
        for (AVSpeechSynthesisVoice* voice in [AVSpeechSynthesisVoice speechVoices]) {
            found.push_back({.id = utf8(voice.identifier), .name = utf8(voice.name), .language = utf8(voice.language)});
        }
    }
    return found;
}

std::vector<std::string> SystemSpeech::languages() const
{
    std::vector<std::string> found;
    for (const SpeechVoice& voice : voices()) {
        if (std::find(found.begin(), found.end(), voice.language) == found.end()) {
            found.push_back(voice.language);
        }
    }
    return found;
}

std::uint64_t SystemSpeech::say(SpeechLine line, Spoken spoken)
{
    @autoreleasepool {
        AVSpeechUtterance* utterance = [AVSpeechUtterance speechUtteranceWithString:nsText(line.text)];
        if (!line.voice.empty()) {
            utterance.voice = [AVSpeechSynthesisVoice voiceWithIdentifier:nsText(line.voice)];
        }
        utterance.rate = static_cast<float>(std::clamp(line.rate, 0.1, 4.0) * AVSpeechUtteranceDefaultSpeechRate);
        utterance.rate = std::clamp(utterance.rate, AVSpeechUtteranceMinimumSpeechRate, AVSpeechUtteranceMaximumSpeechRate);
        utterance.volume = m_impl->silent ? 0.0F : static_cast<float>(std::clamp(line.volume, 0.0, 1.0));
        AVSpeechSynthesizer* speaker = [[AVSpeechSynthesizer alloc] init];
        speaker.delegate = m_impl->delegate;
        std::uint64_t id = 0;
        {
            const std::scoped_lock lock(m_impl->mutex);
            id = m_impl->next++;
            m_impl->spoken[id] = std::move(spoken);
            m_impl->speakers[id] = speaker;
        }
        [speaker speakUtterance:utterance];
        return id;
    }
}

void SystemSpeech::stop(std::uint64_t id)
{
    AVSpeechSynthesizer* speaker = nil;
    {
        const std::scoped_lock lock(m_impl->mutex);
        const auto found = m_impl->speakers.find(id);
        speaker = found != m_impl->speakers.end() ? found->second : nil;
    }
    [speaker stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
}

std::uint64_t SystemSpeech::listen(SpeechCommands, Heard, Stopped stopped)
{
    if (stopped) {
        stopped("voice commands are not on macOS yet");
    }
    return 0;
}

void SystemSpeech::unlisten(std::uint64_t) {}

std::optional<std::vector<std::uint8_t>> SystemSpeech::record(const SpeechLine&)
{
    return std::nullopt;
}

void SystemSpeech::hearFrom(std::vector<std::uint8_t>) {}

}
