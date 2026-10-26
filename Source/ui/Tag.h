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

    /** Big wild-style graffiti lettering: fat chiselled letters that overlap, a deep 3D block extrusion,
        black + white outlines, spray glow, spikes, glints and drips. */
    struct GStyle
    {
        juce::Colour fillTop { 0xffffe14d }, fillBottom { 0xffff7b1c };
        juce::Colour inlineCol { 0xffffffff }, outline { 0xff0b0612 };
        juce::Colour extrudeTop { 0xff8d8c96 }, extrudeBottom { 0xff2a2830 };
        juce::Colour glow { 0xffff2e93 };
        float thickness = 0.22f;      // stroke width relative to letter height
        float outlineW  = 0.06f;
        float inlineW   = 0.03f;
        float depth     = 0.14f;      // length of the 3D extrusion relative to letter height
        float slant     = 0.28f;
        float overlap   = 0.03f;      // letters overlap by this much (relative to height)
        float jitter    = 1.0f;
        float smallScale = 1.0f;
        bool  spikes = true, drips = true, glints = true, spray = true;
        int   seed = 1;
    };
    juce::Rectangle<float> drawGraffiti (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, const GStyle&);

    /** Quick helper for small labels: thin single-colour marker lettering. */
    void drawSimple (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, juce::Colour fill, juce::Colour outline, float thickness = 0.16f);
}
