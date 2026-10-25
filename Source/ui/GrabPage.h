#pragma once
#include "PianoRoll.h"
#include "BonyLookAndFeel.h"

namespace bony
{
class GrabPage : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit GrabPage (BonyProcessor&);
    ~GrabPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void loadFileDialog();
private:
    class DragHandle;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refreshButtons();

    BonyProcessor& proc;
    TimeView view;
    WaveformView wave;
    PianoRoll roll;
    juce::TextButton loadBtn { "LOAD" }, recBtn { "REC" }, fitBtn { "FIT" }, playBtn { "PLAY" }, saveBtn { "SAVE .MID" }, resetBtn { "RESET EDITS" };
    juce::ToggleButton syncBtn { "SYNC REC" }, hearSource { "HEAR SOURCE" };
    juce::Slider grabSl, lowSl, highSl, gainKnob, volKnob;
    juce::ComboBox snapBox, soundBox;
    std::unique_ptr<DragHandle> drag;
    std::shared_ptr<juce::FileChooser> chooser;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> grabA, lowA, highA, gainA, volA;
    std::unique_ptr<CA> snapA, soundA;
    std::unique_ptr<BA> srcA;
    juce::Label lGrab, lLow, lHigh, lGain, lVol, lSnap, lSound, status;
    juce::Rectangle<int> deckArea;
    bool wasBusy = false, fitted = false;
    SourcePtr lastSource;
};
}
