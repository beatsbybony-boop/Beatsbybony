#include "PluginEditor.h"
#include "ui/Art.h"
#include "ui/Tag.h"

namespace bony
{
using namespace juce;
using namespace theme;

class BonyEditor::Content : public Component
{
public:
    Content (BonyProcessor& p) : proc (p), grab (p) { addAndMakeVisible (grab); }

    void paint (Graphics& g) override
    {
        if (bricks.getWidth() != getWidth() || bricks.getHeight() != getHeight())
        {
            bricks = Image (Image::RGB, getWidth(), getHeight(), false);
            Graphics bg (bricks);
            art::drawBricks (bg, bricks.getBounds());
            art::drawBackdrop (bg, getWidth(), getHeight());
            header = Image (Image::ARGB, getWidth(), 112, true);
            Graphics hg (header);
            paintHeader (hg);
        }
        g.drawImageAt (bricks, 0, 0);
        g.drawImageAt (header, 0, 0);
        art::drawCassette (g, Rectangle<float> (getWidth() - 100.0f, 22.0f, 84.0f, 54.0f), "BBB-86", orange, spin, true, false, false);

        // footer
        auto foot = Rectangle<float> (0, (float) getHeight() - 26.0f, (float) getWidth(), 26.0f);
        g.setColour (ink); g.fillRect (foot);
        g.setColour (pink); g.fillRect (foot.withHeight (2.0f));
        g.setColour (cream.withAlpha (0.85f)); g.setFont (display (14.0f));
        g.drawText ("BEATSBYBONY  *  NOTE GRABBER  *  AUDIO > MIDI", foot.reduced (14.0f, 0), Justification::centredLeft);
        g.setColour (cyan); g.drawText ("v1.0", foot.reduced (14.0f, 0), Justification::centredRight);
    }

    void paintHeader (Graphics& g)
    {
        // header band
        auto head = Rectangle<float> (0, 0, (float) getWidth(), 104.0f);
        g.setGradientFill (ColourGradient::vertical (Colour (0xdd1b0a2e), 0, Colour (0xee0b0612), head.getHeight()));
        g.fillRect (head);
        art::drawCheckerStrip (g, Rectangle<float> (0, head.getBottom() - 8.0f, head.getWidth(), 8.0f), pink, ink, 8.0f);
        art::drawLogo (g, Rectangle<float> (26.0f, 26.0f, 520.0f, 74.0f));
        art::drawStar (g, { 562.0f, 20.0f }, 11.0f, yellow, ink, 0.3f);
        art::drawSticker (g, "NOTE GRABBER", Rectangle<float> (590.0f, 14.0f, 190.0f, 22.0f), yellow, ink, -1.5f);
        art::drawSticker (g, "AUDIO > MIDI", Rectangle<float> (612.0f, 48.0f, 150.0f, 20.0f), cyan, ink, 1.5f);

    }

    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromBottom (26);
        auto head = r.removeFromTop (104);
        grab.setBounds (r);
    }

    float spin = 0.0f;
private:
    BonyProcessor& proc;
    GrabPage grab;
    Image bricks, header;
};

BonyEditor::BonyEditor (BonyProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    content = std::make_unique<Content> (p);
    addAndMakeVisible (*content);
    content->setBounds (0, 0, designW, designH);
    setResizable (true, true);
    setResizeLimits (640, (int) (640.0 * designH / designW), 1960, (int) (1960.0 * designH / designW));
    getConstrainer()->setFixedAspectRatio ((double) designW / designH);
    setSize (designW, designH);
    proc.addChangeListener (this);
    startTimerHz (20);
}

BonyEditor::~BonyEditor()
{
    proc.removeChangeListener (this);
    stopTimer();
    setLookAndFeel (nullptr);
}

void BonyEditor::paint (Graphics& g) { g.fillAll (ink); }

void BonyEditor::timerCallback() { content->spin += 0.15f; content->repaint (0, 0, designW, 110); }

void BonyEditor::resized()
{
    const float s = (float) getWidth() / (float) designW;
    content->setTransform (AffineTransform::scale (s));
}

void BonyEditor::filesDropped (const StringArray& files, int, int)
{
    for (auto& f : files)
    {
        File file (f);
        if (file.hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) { proc.loadFileAsync (file); return; }
    }
    proc.clearError();
}
}
