#pragma once
#include "../PluginProcessor.h"
#include "BonyLookAndFeel.h"

namespace bony
{
class BoostPage : public juce::Component, private juce::Timer
{
public:
    explicit BoostPage (BonyProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, float value, const juce::String& label, bool reversed);

    BonyProcessor& proc;
    juce::Slider amount, outTrim;
    juce::TextButton modeBtn[4], powerBtn { "POWER" };
    juce::Label lOut;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SA> amountA, outA;
    juce::Rectangle<float> knobArea;
    float phase = 0.0f, smoothIn = 0.0f, smoothOut = 0.0f, smoothGr = 0.0f;
    static constexpr const char* modeNames[4] = { "A", "B", "C", "D" };
};
}
