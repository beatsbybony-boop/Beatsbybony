#include "GrabEngine.h"
#include "NoteDetector.h"

namespace bony
{
void GrabEngine::analyse (SourcePtr source, float gainDb)
{
    { const juce::ScopedLock sl (lock); job = std::move (source); jobGain = gainDb; }
    ++generation;                       // tells a running job to stop
    busy = true;
    if (! isThreadRunning()) startThread (juce::Thread::Priority::low);
    else notify();
}

void GrabEngine::clear()
{
    ++generation;
    { const juce::ScopedLock sl (lock); notes.clear(); job.reset(); }
    sendChangeMessage();
}

void GrabEngine::run()
{
    while (! threadShouldExit())
    {
        SourcePtr src; float gain; int gen;
        { const juce::ScopedLock sl (lock); src = job; gain = jobGain; gen = generation.load(); }
        if (src == nullptr || src->audio.getNumSamples() == 0) { busy = false; return; }

        const int n = src->audio.getNumSamples();
        std::vector<float> mono ((size_t) n, 0.0f);
        const int chans = src->audio.getNumChannels();
        for (int c = 0; c < chans; ++c)
        {
            const float* p = src->audio.getReadPointer (c);
            for (int i = 0; i < n; ++i) mono[(size_t) i] += p[i] / (float) chans;
        }

        prog = 0.0f;
        DetectorSettings st; st.gainDb = gain; st.lowestNote = 24; st.highestNote = 108;
        auto result = detectNotes (mono.data(), mono.size(), src->sampleRate, st,
                                   [&] (float p) { prog = p; return generation.load() == gen && ! threadShouldExit(); });

        if (generation.load() == gen)
        {
            { const juce::ScopedLock sl (lock); notes = std::move (result); job.reset(); }
            busy = false;
            prog = 1.0f;
            sendChangeMessage();
            return;
        }
        // a newer request arrived: loop and run it
    }
    busy = false;
}

bool GrabEngine::isActive (const GrabNote& n, float sens, int lo, int hi) const
{
    if (n.manual > 0) return true;                 // a note you clicked on stays on, whatever the sliders say
    if (n.manual < 0) return false;
    if (n.midi < lo || n.midi > hi) return false;
    return n.strength >= thresholdFor (sens);
}

std::vector<GrabNote> GrabEngine::getActive (float sens, int lo, int hi) const
{
    std::vector<GrabNote> out;
    const juce::ScopedLock sl (lock);
    for (auto& n : notes) if (isActive (n, sens, lo, hi)) out.push_back (n);
    return out;
}

void GrabEngine::toggle (int index, float sens, int lo, int hi)
{
    { const juce::ScopedLock sl (lock);
      if (index < 0 || index >= (int) notes.size()) return;
      auto& n = notes[(size_t) index];
      n.manual = isActive (n, sens, lo, hi) ? -1 : +1; }
    sendChangeMessage();
}

void GrabEngine::resetEdits()
{
    { const juce::ScopedLock sl (lock); for (auto& n : notes) n.manual = 0; }
    sendChangeMessage();
}
}
