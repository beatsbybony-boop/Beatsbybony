#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace bony::ids
{
    // GRAB (audio -> MIDI)
    inline const juce::String grabSens   { "grabSens" };    // quickGRAB slider
    inline const juce::String grabLow    { "grabLow" };     // lowest note
    inline const juce::String grabHigh   { "grabHigh" };    // highest note
    inline const juce::String grabGain   { "grabGain" };    // analysis gain (dB)
    inline const juce::String snap       { "snap" };        // quantise on export
    inline const juce::String prevSound  { "prevSound" };
    inline const juce::String prevVol    { "prevVol" };
    inline const juce::String prevSource { "prevSource" };  // also hear the original audio
}

namespace bony
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::grabSens, 1 }, "quickGRAB",
                                                   NormalisableRange<float> (0.0f, 1.0f), 0.55f));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { ids::grabLow, 1 },  "Range Low",  12, 108, 36));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { ids::grabHigh, 1 }, "Range High", 12, 108, 96));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::grabGain, 1 }, "Grab Gain",
                                                   NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::snap, 1 }, "Snap",
                                                    StringArray { "Off", "1/4", "1/8", "1/16", "1/32" }, 3));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::prevSound, 1 }, "Preview Sound",
                                                    StringArray { "Keys", "Saw", "Square", "Sine" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::prevVol, 1 }, "Preview Volume",
                                                   NormalisableRange<float> (0.0f, 1.0f), 0.7f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ids::prevSource, 1 }, "Hear Source", true));

    return l;
}
}
