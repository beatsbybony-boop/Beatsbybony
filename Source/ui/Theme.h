#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace bony::theme
{
    using juce::Colour;
    inline const Colour ink      { 0xff0b0612 };
    inline const Colour bg0      { 0xff10071c };
    inline const Colour bg1      { 0xff1d0c32 };
    inline const Colour panel    { 0xff180a2a };
    inline const Colour pink     { 0xffff2e93 };
    inline const Colour cyan     { 0xff1ee6ff };
    inline const Colour yellow   { 0xffffd60a };
    inline const Colour orange   { 0xffff7b1c };
    inline const Colour lime     { 0xffa6ff3c };
    inline const Colour violet   { 0xff8a3dff };
    inline const Colour gold     { 0xffffc21a };
    inline const Colour goldDark { 0xffb87a08 };
    inline const Colour cream    { 0xfff5edd6 };
    inline const Colour dim      { 0xff5b4a78 };

    /** Chunky poster font. Impact is on every Windows machine; elsewhere we fall back to a bold sans. */
    juce::Font display (float height);
    juce::Font body (float height, bool bold = false);
}
