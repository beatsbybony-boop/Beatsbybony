#pragma once
#include <vector>

namespace bony
{
struct GrabNote
{
    double start = 0.0;     // seconds
    double length = 0.0;    // seconds
    int    midi = 60;
    float  strength = 0.0f; // 0..1, how clearly the note was heard (used by the quickGRAB slider)
    float  velocity = 0.8f; // 0..1
    int    manual = 0;      // +1 = forced on, -1 = forced off, 0 = follow the slider
};
}
