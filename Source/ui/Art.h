#pragma once
#include "Theme.h"

/** All the 80s hip hop artwork is drawn with code (no image files needed). */
namespace bony::art
{
    struct GraffitiStyle
    {
        juce::Colour fillTop, fillBottom, outline, rim, shadow;
        float outlineWidth = 6.0f;
        float rimWidth = 3.0f;
        float shadowDx = 4.0f, shadowDy = 5.0f;
        float slant = 0.18f;          // italic lean
        float rotationDeg = -2.0f;
        bool  drips = true;
        int   seed = 1;
    };

    void drawBricks (juce::Graphics&, juce::Rectangle<int> area, int seed = 7);
    void drawCrown (juce::Graphics&, juce::Rectangle<float> area, bool glow = true);
    void drawCassette (juce::Graphics&, juce::Rectangle<float> area, const juce::String& label,
                       juce::Colour accent, float reelAngle, bool lit, bool empty, bool withCrown);
    void drawGraffitiText (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, const GraffitiStyle&);
    void drawLogo (juce::Graphics&, juce::Rectangle<float> area);
    void drawBackdrop (juce::Graphics&, int width, int height);
    void drawSplat (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour, int seed);
    void drawStar (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour fill, juce::Colour outline, float rotation = 0.0f, int points = 5);
    void drawBolt (juce::Graphics&, juce::Rectangle<float> area, juce::Colour fill);
    void drawSticker (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, juce::Colour fill, juce::Colour text_, float rotationDeg);
    void drawSpeaker (juce::Graphics&, juce::Rectangle<float> area, float pulse);
    void drawNeonFrame (juce::Graphics&, juce::Rectangle<float> area, juce::Colour c, float corner = 10.0f, float thickness = 2.0f);
    void drawDeck (juce::Graphics&, juce::Rectangle<float> area, const juce::String& title, juce::Colour accent);
    void drawTapeCounter (juce::Graphics&, juce::Rectangle<float> area, float progress01, const juce::String& text);
    void drawCheckerStrip (juce::Graphics&, juce::Rectangle<float> area, juce::Colour a, juce::Colour b, float size);
}
