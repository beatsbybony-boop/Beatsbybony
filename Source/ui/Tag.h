#pragma once
#include "Theme.h"

/** Hand-built graffiti lettering. Every letter is a thick marker stroke, so it works on any computer
    (no font files needed). */
namespace bony::tag
{
    struct Style
    {
        juce::Colour fillTop { 0xffff7dc0 }, fillBottom { 0xffff2e93 };
        juce::Colour outline { 0xff0b0612 }, rim { 0xffffffff }, shadow { 0xff6a1fd0 };
        float thickness = 0.20f;      // stroke width relative to letter height
        float outlineWidth = 0.10f;   // black edge, relative to letter height
        float rimWidth = 0.035f;      // white inline edge
        float shadowDx = 0.07f, shadowDy = 0.09f;   // relative to height
        float slant = 0.22f;
        float jitter = 1.0f;          // how bouncy the letters are
        float smallScale = 1.0f;      // letters after the first of each word are scaled by this (1 = all same)
        bool  drips = false;
        bool  shine = true;
        int   seed = 1;
    };

    /** Draws text (A-Z, 0-9 and a few symbols) fitted into the area. Returns the bounds actually used. */
    juce::Rectangle<float> draw (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, const Style&);

    /** Quick helper for small labels: thin single-colour marker lettering. */
    void drawSimple (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, juce::Colour fill, juce::Colour outline, float thickness = 0.16f);
}
