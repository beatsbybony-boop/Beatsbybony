#pragma once
#include "../PluginProcessor.h"
#include "BonyLookAndFeel.h"

namespace bony
{
class SplitPage : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit SplitPage (BonyProcessor&);
    ~SplitPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    class Tape;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override;
    void refresh();

    BonyProcessor& proc;
    juce::OwnedArray<Tape> tapes;
    juce::TextButton dlBtn { "DOWNLOAD 170 MB" }, goBtn { "SPLIT IT!" }, playBtn { "PLAY" }, saveAllBtn { "SAVE ALL" }, folderBtn { "MODELS FOLDER" };
    juce::ToggleButton optInstr { "DRUM/BASS/OTHER" }, optVocal { "LEAD/BACKING VOX" }, optQuick { "QUICK TEST 30s" };
    std::shared_ptr<juce::FileChooser> chooser;
    juce::String counterText; float counterValue = 0.0f;
    float spin = 0.0f;
};
}
