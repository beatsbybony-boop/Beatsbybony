#pragma once
#include "../Types.h"
#include "ModelManager.h"
#include <juce_events/juce_events.h>
#include <atomic>

namespace bony
{
class StemEngine : public juce::ChangeBroadcaster, private juce::Thread
{
public:
    struct Options { bool instruments = true; bool vocalSplit = true; bool quickTest = false; };
    enum class State { Idle, Working, Done, Failed };

    explicit StemEngine (ModelManager& m) : juce::Thread ("BeatsByBony stems"), models (m) {}
    ~StemEngine() override { stopThread (8000); }

    void start (SourcePtr source, Options opts);
    void cancel() { signalThreadShouldExit(); }

    State getState() const { return state.load(); }
    float progress() const { return prog.load(); }
    juce::String statusText() const { const juce::ScopedLock sl (lock); return status; }
    juce::String errorText() const { const juce::ScopedLock sl (lock); return error; }
    StemSetPtr getResult() const { const juce::ScopedLock sl (lock); return result; }
    void clearResult() { { const juce::ScopedLock sl (lock); result.reset(); } state = State::Idle; sendChangeMessage(); }

private:
    void run() override;
    void setStatus (const juce::String& s) { { const juce::ScopedLock sl (lock); status = s; } sendChangeMessage(); }
    void fail (const juce::String& e) { { const juce::ScopedLock sl (lock); error = e; status = "Something went wrong"; } state = State::Failed; sendChangeMessage(); }

    ModelManager& models;
    SourcePtr source; Options opts;
    mutable juce::CriticalSection lock;
    juce::String status, error;
    StemSetPtr result;
    std::atomic<State> state { State::Idle };
    std::atomic<float> prog { 0.0f };
};
}
