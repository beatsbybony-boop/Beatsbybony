#include "GrabPage.h"
#include "Art.h"

namespace bony
{
using namespace juce;
using namespace theme;

class GrabPage::DragHandle : public Component
{
public:
    DragHandle (BonyProcessor& p) : proc (p) { setMouseCursor (MouseCursor::DraggingHandCursor); }
    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        art::drawCheckerStrip (g, b.removeFromTop (6.0f), pink, ink, 6.0f);
        g.setColour (ink); g.fillRoundedRectangle (b, 8.0f);
        art::drawCassette (g, Rectangle<float> (10.0f, b.getY() + 4.0f, 86.0f, b.getHeight() - 10.0f), "MIDI", yellow, angle, hot, false, false);
        g.setColour (hot ? yellow : cream); g.setFont (display (24.0f));
        g.drawText ("DRAG MIDI  >>  FL PIANO ROLL", 108, (int) b.getY(), getWidth() - 112, (int) b.getHeight() - 14, Justification::centredLeft);
        g.setColour (dim.brighter (0.5f)); g.setFont (body (13.0f, true));
        g.drawText (String (count) + " notes ready  -  grab it and drop it on a pattern", 108, (int) b.getBottom() - 24, getWidth() - 112, 18, Justification::centredLeft);
    }
    void mouseEnter (const MouseEvent&) override { hot = true; repaint(); }
    void mouseExit (const MouseEvent&) override { hot = false; repaint(); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() < 8 || dragging) return;
        const auto f = proc.writeMidiToTemp();
        if (! f.existsAsFile()) return;
        dragging = true;
        if (auto* c = DragAndDropContainer::findParentDragContainerFor (this))
            c->performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
        else dragging = false;
    }
    int count = 0; float angle = 0.0f;
private:
    BonyProcessor& proc; bool hot = false, dragging = false;
};

static void styleLabel (Label& l, const String& t, Justification j = Justification::centred, float size = 15.0f)
{
    l.setText (t, dontSendNotification); l.setJustificationType (j); l.setFont (display (size)); l.setColour (Label::textColourId, cream);
    l.setInterceptsMouseClicks (false, false);
}

GrabPage::GrabPage (BonyProcessor& p) : proc (p), wave (p, view), roll (p, view)
{
    addAndMakeVisible (wave); addAndMakeVisible (roll);
    drag = std::make_unique<DragHandle> (p); addAndMakeVisible (*drag);
    roll.onViewChanged = [this] { wave.repaint(); };

    for (auto* b : { &loadBtn, &recBtn, &fitBtn, &playBtn, &saveBtn, &resetBtn }) addAndMakeVisible (b);
    loadBtn.setColour (TextButton::buttonColourId, cyan);
    recBtn.setColour (TextButton::buttonColourId, pink);
    fitBtn.setColour (TextButton::buttonColourId, lime);
    playBtn.setColour (TextButton::buttonColourId, yellow); playBtn.setColour (TextButton::buttonOnColourId, orange);
    saveBtn.setColour (TextButton::buttonColourId, cyan);
    resetBtn.setColour (TextButton::buttonColourId, violet.brighter (0.3f));
    addAndMakeVisible (syncBtn); addAndMakeVisible (hearSource);
    syncBtn.setColour (ToggleButton::tickColourId, pink); hearSource.setColour (ToggleButton::tickColourId, cyan);
    syncBtn.setTooltip ("Armed: recording starts when you press play in FL Studio and stops when you stop it");

    auto setupV = [this] (Slider& s, Colour c) { s.setSliderStyle (Slider::LinearVertical); s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0); s.setColour (Slider::thumbColourId, c); addAndMakeVisible (s); };
    setupV (grabSl, pink); setupV (lowSl, cyan); setupV (highSl, lime);
    auto setupK = [this] (Slider& s, Colour c) { s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true); s.setColour (Slider::rotarySliderFillColourId, c); addAndMakeVisible (s); };
    setupK (gainKnob, yellow); setupK (volKnob, orange);

    grabSl.setTooltip ("quickGRAB: slide up to grab more notes, down for only the clearest ones");
    for (auto* s : { &lowSl, &highSl })
    {
        s->textFromValueFunction = [] (double v) { return MidiMessage::getMidiNoteName ((int) v, true, true, 5); };
    }
    gainKnob.textFromValueFunction = [] (double v) { return String (v, 1) + " dB"; };

    addAndMakeVisible (snapBox); addAndMakeVisible (soundBox);
    snapBox.addItemList ({ "Off", "1/4", "1/8", "1/16", "1/32" }, 1);
    soundBox.addItemList ({ "Keys", "Saw", "Square", "Sine" }, 1);

    grabA = std::make_unique<SA> (proc.apvts, ids::grabSens, grabSl);
    lowA  = std::make_unique<SA> (proc.apvts, ids::grabLow, lowSl);
    highA = std::make_unique<SA> (proc.apvts, ids::grabHigh, highSl);
    gainA = std::make_unique<SA> (proc.apvts, ids::grabGain, gainKnob);
    volA  = std::make_unique<SA> (proc.apvts, ids::prevVol, volKnob);
    snapA = std::make_unique<CA> (proc.apvts, ids::snap, snapBox);
    soundA = std::make_unique<CA> (proc.apvts, ids::prevSound, soundBox);
    srcA = std::make_unique<BA> (proc.apvts, ids::prevSource, hearSource);

    styleLabel (lGrab, "quickGRAB", Justification::centred, 17.0f); lGrab.setColour (Label::textColourId, pink);
    styleLabel (lLow, "LOW"); styleLabel (lHigh, "HIGH"); styleLabel (lGain, "GAIN"); styleLabel (lVol, "VOLUME");
    styleLabel (lSnap, "SNAP"); styleLabel (lSound, "SOUND");
    for (auto* l : { &lGrab, &lLow, &lHigh, &lGain, &lVol, &lSnap, &lSound }) addAndMakeVisible (l);
    status.setFont (body (13.0f, true)); status.setColour (Label::textColourId, yellow); status.setJustificationType (Justification::centredLeft);
    addAndMakeVisible (status);

    loadBtn.onClick = [this] { loadFileDialog(); };
    recBtn.onClick = [this] { if (proc.isCapturing()) proc.stopCaptureNow(); else proc.startCaptureNow(); };
    syncBtn.onClick = [this] { proc.armCapture (syncBtn.getToggleState()); };
    fitBtn.onClick = [this] { roll.zoomToFit(); };
    resetBtn.onClick = [this] { proc.grab.resetEdits(); };
    playBtn.onClick = [this] {
        if (proc.getPlayMode() == PlayMode::Grab) proc.setPlayMode (PlayMode::Off);
        else { proc.setPlayMode (PlayMode::Grab); } };
    saveBtn.onClick = [this]
    {
        auto f = proc.writeMidiToTemp();
        if (! f.existsAsFile()) { status.setText ("Nothing to save yet.", dontSendNotification); return; }
        chooser = std::make_shared<FileChooser> ("Save MIDI", File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (f.getFileName()), "*.mid");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, f] (const FileChooser& fc)
        { auto r = fc.getResult(); if (r != File()) { f.copyFileTo (r.withFileExtension ("mid")); status.setText ("Saved " + r.getFileName(), dontSendNotification); } });
    };

    proc.addChangeListener (this);
    startTimerHz (15);
    refreshButtons();
}

GrabPage::~GrabPage() { proc.removeChangeListener (this); }

void GrabPage::loadFileDialog()
{
    chooser = std::make_shared<FileChooser> ("Pick a sample", File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.mp3;*.aif;*.aiff;*.flac;*.ogg");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
    { auto f = fc.getResult(); if (f.existsAsFile()) proc.loadFileAsync (f); });
}

void GrabPage::changeListenerCallback (ChangeBroadcaster*)
{
    const bool busy = proc.grab.isBusy();
    const auto src = proc.getSource();
    if ((wasBusy && ! busy) || src != lastSource) { lastSource = src; if (! busy) roll.zoomToFit(); }
    wasBusy = busy;
    refreshButtons(); repaint();
}

void GrabPage::timerCallback()
{
    drag->angle += 0.25f; if (proc.getPlayMode() == PlayMode::Grab) drag->repaint();
    if (proc.isCapturing()) refreshButtons();
}

void GrabPage::refreshButtons()
{
    recBtn.setButtonText (proc.isCapturing() ? "STOP " + String (proc.captureSeconds(), 1) + "s" : "REC");
    playBtn.setButtonText (proc.getPlayMode() == PlayMode::Grab ? "STOP" : "PLAY");
    playBtn.setToggleState (proc.getPlayMode() == PlayMode::Grab, dontSendNotification);
    syncBtn.setToggleState (proc.isArmed(), dontSendNotification);
    const float sens = grabSl.getValue();
    drag->count = (int) proc.grab.getActive ((float) sens, (int) lowSl.getValue(), (int) highSl.getValue()).size();
    drag->repaint();
    auto err = proc.lastError();
    if (err.isNotEmpty()) status.setText (err, dontSendNotification);
    else if (proc.isCapturing()) status.setText ("Recording what comes into this plugin...", dontSendNotification);
    else if (proc.grab.isBusy()) status.setText ("Grabbing notes...", dontSendNotification);
    else if (proc.getSource()) status.setText (String (drag->count) + " of " + String (proc.grab.size()) + " notes grabbed. Click notes to switch them on/off.", dontSendNotification);
    else status.setText ("Drop a sample on the plugin, hit LOAD, or REC what you are playing.", dontSendNotification);
    wave.repaint(); roll.repaint();
}

void GrabPage::paint (Graphics& g)
{
    art::drawDeck (g, deckArea.toFloat(), "CONTROLS", pink);
    g.setColour (violet.brighter (0.3f)); g.setFont (body (12.0f, true));
    g.drawText (MidiMessage::getMidiNoteName ((int) lowSl.getValue(), true, true, 5), lowSl.getX() - 6, lowSl.getBottom() + 0, lowSl.getWidth() + 12, 16, Justification::centred);
    g.drawText (MidiMessage::getMidiNoteName ((int) highSl.getValue(), true, true, 5), highSl.getX() - 6, highSl.getBottom() + 0, highSl.getWidth() + 12, 16, Justification::centred);
    g.setColour (pink); g.drawText (String ((int) (grabSl.getValue() * 100)) + "%", grabSl.getX(), grabSl.getBottom(), grabSl.getWidth(), 16, Justification::centred);
    g.setColour (yellow); g.drawText (String (gainKnob.getValue(), 1) + " dB", gainKnob.getX(), gainKnob.getBottom() - 4, gainKnob.getWidth(), 14, Justification::centred);
    art::drawBolt (g, Rectangle<float> ((float) deckArea.getRight() - 30.0f, (float) deckArea.getY() - 14.0f, 20.0f, 28.0f), yellow);
}

void GrabPage::resized()
{
    auto r = getLocalBounds().reduced (12, 6);
    deckArea = r.removeFromRight (258).withTrimmedTop (8).withTrimmedBottom (4);
    r.removeFromRight (10);

    auto top = r.removeFromTop (34);
    loadBtn.setBounds (top.removeFromLeft (84)); top.removeFromLeft (6);
    recBtn.setBounds (top.removeFromLeft (110)); top.removeFromLeft (8);
    syncBtn.setBounds (top.removeFromLeft (170));
    fitBtn.setBounds (top.removeFromRight (64)); top.removeFromRight (6);
    resetBtn.setBounds (top.removeFromRight (130));
    r.removeFromTop (6);
    wave.setBounds (r.removeFromTop (92)); r.removeFromTop (6);
    auto bottom = r.removeFromBottom (74);
    status.setBounds (r.removeFromBottom (20));
    roll.setBounds (r);
    drag->setBounds (bottom.removeFromLeft (bottom.getWidth() - 130).withTrimmedTop (4)); bottom.removeFromLeft (6);
    saveBtn.setBounds (bottom.withTrimmedTop (14).withHeight (46));

    // deck
    auto d = deckArea.reduced (12, 14);
    d.removeFromTop (10);
    auto sliders = d.removeFromTop (210);
    auto col = [&] (int w) { auto c = sliders.removeFromLeft (w); return c; };
    auto c1 = col (96); lGrab.setBounds (c1.removeFromTop (22)); grabSl.setBounds (c1.withTrimmedBottom (18));
    auto c2 = col (66); lLow.setBounds (c2.removeFromTop (22)); lowSl.setBounds (c2.withTrimmedBottom (18));
    auto c3 = col (66); lHigh.setBounds (c3.removeFromTop (22)); highSl.setBounds (c3.withTrimmedBottom (18));
    d.removeFromTop (4);
    auto knobs = d.removeFromTop (86);
    auto k1 = knobs.removeFromLeft (110); lGain.setBounds (k1.removeFromTop (16)); gainKnob.setBounds (k1.withTrimmedBottom (10).reduced (14, 0));
    auto k2 = knobs; lVol.setBounds (k2.removeFromTop (16)); volKnob.setBounds (k2.withTrimmedBottom (10).reduced (14, 0));
    d.removeFromTop (2);
    auto combos = d.removeFromTop (46);
    auto s1 = combos.removeFromLeft (110); lSnap.setBounds (s1.removeFromTop (16)); snapBox.setBounds (s1.reduced (2, 0).withHeight (26));
    combos.removeFromLeft (6);
    lSound.setBounds (combos.removeFromTop (16)); soundBox.setBounds (combos.reduced (2, 0).withHeight (26));
    hearSource.setBounds (d.removeFromTop (28)); d.removeFromTop (4);
    playBtn.setBounds (d.removeFromTop (46));
    if (! fitted && roll.getWidth() > 0) { fitted = true; roll.zoomToFit(); }
}
}
