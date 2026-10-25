#pragma once
#include "../Types.h"
#include "GrabTypes.h"
#include <juce_events/juce_events.h>
#include <atomic>

namespace bony
{
/** Runs the note detection in the background and holds the results. */
class GrabEngine : public juce::ChangeBroadcaster, private juce::Thread
{
public:
    GrabEngine() : juce::Thread ("BeatsByBony note grabber") {}
    ~GrabEngine() override { stopThread (3000); }

    void analyse (SourcePtr source, float gainDb);
    void clear();
    bool isBusy() const { return busy.load(); }
    float progress() const { return prog.load(); }

    std::vector<GrabNote> getAll() const { const juce::ScopedLock sl (lock); return notes; }
    std::vector<GrabNote> getActive (float sens, int lo, int hi) const;
    int  size() const { const juce::ScopedLock sl (lock); return (int) notes.size(); }
    bool isActive (const GrabNote& n, float sens, int lo, int hi) const;
    void toggle (int index, float sens, int lo, int hi);
    void resetEdits();
    static float thresholdFor (float sens) { return 0.035f + 0.75f * (1.0f - sens) * (1.0f - sens); }

private:
    void run() override;

    mutable juce::CriticalSection lock;
    std::vector<GrabNote> notes;
    SourcePtr job; float jobGain = 0.0f;
    std::atomic<bool> busy { false };
    std::atomic<float> prog { 0.0f };
    std::atomic<int> generation { 0 };
};
}
