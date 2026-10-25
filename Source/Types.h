#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <vector>

namespace bony
{
/** Audio the user loaded or captured. Never modified once created. */
struct SourceAudio
{
    juce::AudioBuffer<float> audio;     // 1 or 2 channels
    double sampleRate = 44100.0;
    juce::String name;
    juce::File file;                    // empty for captured audio
    double seconds() const { return sampleRate > 0 ? audio.getNumSamples() / sampleRate : 0.0; }
};
using SourcePtr = std::shared_ptr<const SourceAudio>;

struct StemSet
{
    struct Stem { juce::String name; juce::AudioBuffer<float> audio; bool present = false; };   // stereo @ sampleRate
    std::vector<Stem> stems;
    double sampleRate = 44100.0;
    juce::String sourceName;
    int numSamples() const { return stems.empty() ? 0 : stems.front().audio.getNumSamples(); }
};
using StemSetPtr = std::shared_ptr<const StemSet>;

/** Fixed order of the six cassettes in the UI. */
enum StemSlot { kVocals = 0, kLead, kBacking, kDrums, kBass, kOther, kNumSlots };
inline const char* stemSlotName (int s)
{
    static const char* n[] = { "VOCALS", "LEAD VOCALS", "BACKING VOCALS", "DRUMS", "BASS", "OTHER" };
    return n[s];
}
}
