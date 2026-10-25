#pragma once
#include "GrabTypes.h"
#include <functional>

namespace bony
{
struct DetectorSettings
{
    float gainDb = 0.0f;     // pushes quiet audio above the noise floor
    int   lowestNote = 24;
    int   highestNote = 108;
};

/** Finds (polyphonic) notes in audio. Returns every plausible note with a strength value; the
    quickGRAB slider later keeps only the strongest ones, so the slider is instant.
    progress returns false to cancel. */
std::vector<GrabNote> detectNotes (const float* mono, size_t numSamples, double sampleRate,
                                   const DetectorSettings& settings,
                                   const std::function<bool (float)>& progress);
}
