#pragma once
#include "GrabTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace bony
{
/** snapDivision: 0 = off, otherwise notes per whole note (4 = quarter, 8 = eighth, 16...). */
inline bool writeMidiFile (const std::vector<GrabNote>& notes, double bpm, int snapDivision, const juce::File& file)
{
    if (notes.empty()) return false;
    constexpr int ppq = 960;
    bpm = bpm > 20.0 ? bpm : 120.0;

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / bpm)), 0.0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "BeatsByBony"), 0.0);

    const double ticksPerSec = bpm / 60.0 * ppq;
    const double gridTicks = snapDivision > 0 ? (4.0 / snapDivision) * ppq : 0.0;

    for (auto n : notes)
    {
        double on = n.start * ticksPerSec;
        double off = (n.start + n.length) * ticksPerSec;
        if (gridTicks > 0.0)
        {
            on = std::round (on / gridTicks) * gridTicks;
            off = std::round (off / gridTicks) * gridTicks;
            if (off <= on) off = on + gridTicks;
        }
        const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::lround (n.velocity * 127.0f));
        seq.addEvent (juce::MidiMessage::noteOn (1, n.midi, vel), on);
        seq.addEvent (juce::MidiMessage::noteOff (1, n.midi), off);
    }
    seq.updateMatchedPairs();
    seq.sort();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ppq);
    mf.addTrack (seq);
    file.deleteFile();
    juce::FileOutputStream out (file);
    return out.openedOk() && mf.writeTo (out);
}
}
