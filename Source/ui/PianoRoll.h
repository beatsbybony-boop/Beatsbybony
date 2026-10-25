#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"

namespace bony
{
/** Horizontal zoom/scroll shared by the waveform and the piano roll so they line up. */
struct TimeView
{
    double startSec = 0.0;
    double pixelsPerSec = 80.0;
    int leftMargin = 46;
    float xOf (double sec) const { return (float) (leftMargin + (sec - startSec) * pixelsPerSec); }
    double secOf (float x) const { return startSec + (x - leftMargin) / pixelsPerSec; }
};

class WaveformView : public juce::Component, private juce::Timer
{
public:
    WaveformView (BonyProcessor&, TimeView&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    BonyProcessor& proc; TimeView& view;
};

class PianoRoll : public juce::Component, private juce::Timer
{
public:
    PianoRoll (BonyProcessor&, TimeView&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void zoomToFit();
    std::function<void()> onViewChanged;
private:
    void timerCallback() override;
    int noteAt (juce::Point<float>) const;
    float yOfNote (int midi) const;
    void currentParams (float& sens, int& lo, int& hi) const;

    BonyProcessor& proc; TimeView& view;
    int lowNote = 36, highNote = 84;
    int hover = -1;
    double lastPos = -1.0;
};
}
