#pragma once
#include "PluginProcessor.h"
#include "ui/BonyLookAndFeel.h"
#include "ui/GrabPage.h"
#include "ui/SplitPage.h"
#include "ui/BoostPage.h"

namespace bony
{
class BonyEditor : public juce::AudioProcessorEditor,
                   public juce::DragAndDropContainer,
                   public juce::FileDragAndDropTarget,
                   private juce::ChangeListener,
                   private juce::Timer
{
public:
    explicit BonyEditor (BonyProcessor&);
    ~BonyEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray&, int, int) override;
    void showPage (int);

    static constexpr int designW = 980, designH = 670;
private:
    class Content;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    void timerCallback() override;

    BonyProcessor& proc;
    BonyLookAndFeel laf;
    std::unique_ptr<Content> content;
    juce::TooltipWindow tooltips { this, 600 };
};
}
