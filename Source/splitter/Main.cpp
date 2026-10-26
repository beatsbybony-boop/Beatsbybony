#include <juce_gui_extra/juce_gui_extra.h>
#include "SplitterCore.h"
#include "SplitterPage.h"
#include "../stems/Log.h"
#include "../ui/Art.h"
#include "../ui/BonyLookAndFeel.h"

using namespace juce;
using namespace bony;
using namespace bony::theme;

namespace
{
constexpr int designW = 980, designH = 670;

void crashHandler (void*)
{
    logLine ("CRASH: the program stopped unexpectedly. The line above this one is the last step it reached.");
}

/** The whole window: header with the logo, the stem splitter page, a footer. Drawn at a fixed size and scaled to fit. */
class Content : public Component, public DragAndDropContainer, public FileDragAndDropTarget, private Timer
{
public:
    Content() : page (core)
    {
        addAndMakeVisible (page);
        setSize (designW, designH);
        startTimerHz (20);
    }

    bool isInterestedInFileDrag (const StringArray&) override { return true; }
    void filesDropped (const StringArray& files, int, int) override
    {
        for (auto& f : files)
            if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) { core.loadFileAsync (File (f)); return; }
    }

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
            auto head = Rectangle<float> (0, 0, (float) getWidth(), 104.0f);
            hg.setGradientFill (ColourGradient::vertical (Colour (0xdd1b0a2e), 0, Colour (0xee0b0612), head.getHeight()));
            hg.fillRect (head);
            art::drawCheckerStrip (hg, Rectangle<float> (0, head.getBottom() - 8.0f, head.getWidth(), 8.0f), pink, ink, 8.0f);
            art::drawLogo (hg, Rectangle<float> (26.0f, 26.0f, 520.0f, 74.0f));
            art::drawStar (hg, { 562.0f, 20.0f }, 11.0f, yellow, ink, 0.3f);
            art::drawSticker (hg, "STEM SPLITTER", Rectangle<float> (590.0f, 14.0f, 190.0f, 22.0f), yellow, ink, -1.5f);
            art::drawSticker (hg, "VOCALS * DRUMS * BASS", Rectangle<float> (612.0f, 48.0f, 210.0f, 20.0f), cyan, ink, 1.5f);
        }
        g.drawImageAt (bricks, 0, 0);
        g.drawImageAt (header, 0, 0);
        art::drawCassette (g, Rectangle<float> (getWidth() - 100.0f, 22.0f, 84.0f, 54.0f), "BBB-86", orange, spin, true, false, false);

        auto foot = Rectangle<float> (0, (float) getHeight() - 26.0f, (float) getWidth(), 26.0f);
        g.setColour (ink); g.fillRect (foot);
        g.setColour (pink); g.fillRect (foot.withHeight (2.0f));
        g.setColour (cream.withAlpha (0.85f)); g.setFont (display (14.0f));
        g.drawText ("BEATSBYBONY  *  STEM SPLITTER", foot.reduced (14.0f, 0), Justification::centredLeft);
        g.setColour (cyan); g.drawText ("v1.0", foot.reduced (14.0f, 0), Justification::centredRight);
    }

    void resized() override
    {
        page.setBounds (0, 104, designW, designH - 104 - 26);
    }

    float spin = 0.0f;
    SplitterCore core;
private:
    void timerCallback() override { spin += 0.15f; repaint (0, 0, designW, 110); }
    SplitterPage page;
    Image bricks, header;
};

/** Holds Content at its design size and scales it to whatever size the window has. */
class Scaler : public Component
{
public:
    Scaler() { addAndMakeVisible (content); setSize (designW, designH); }
    void paint (Graphics& g) override { g.fillAll (ink); }
    void resized() override
    {
        const float s = jmin ((float) getWidth() / designW, (float) getHeight() / designH);
        content.setTransform (AffineTransform::scale (s).translated (((float) getWidth() - designW * s) * 0.5f, ((float) getHeight() - designH * s) * 0.5f));
    }
    Content content;
};

class MainWindow : public DocumentWindow
{
public:
    MainWindow() : DocumentWindow ("BeatsByBony Stem Splitter", ink, DocumentWindow::allButtons)
    {
        setLookAndFeel (&laf);
        setUsingNativeTitleBar (true);
        setContentOwned (new Scaler(), false);
        setResizable (true, true);
        setResizeLimits (640, 450, 2400, 1700);
        centreWithSize (designW, designH + 30);
        setVisible (true);
    }
    ~MainWindow() override { setLookAndFeel (nullptr); }
    void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }
private:
    BonyLookAndFeel laf;
};

class SplitterApp : public JUCEApplication
{
public:
    const String getApplicationName() override { return "BeatsByBony Stem Splitter"; }
    const String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const String& commandLine) override
    {
        SystemStats::setApplicationCrashHandler (crashHandler);
        logLine ("---- started ----");
        window = std::make_unique<MainWindow>();
        // a song dragged onto the .exe (or "Open with") arrives here
        const auto path = commandLine.unquoted().trim();
        if (path.isNotEmpty() && File (path).existsAsFile())
            if (auto* s = dynamic_cast<Scaler*> (window->getContentComponent())) s->content.core.loadFileAsync (File (path));
    }
    void shutdown() override { window.reset(); logLine ("---- closed ----"); }
    void systemRequestedQuit() override { quit(); }
private:
    std::unique_ptr<MainWindow> window;
};
}

START_JUCE_APPLICATION (SplitterApp)
