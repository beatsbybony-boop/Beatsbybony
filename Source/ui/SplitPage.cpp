#include "SplitPage.h"
#include "Art.h"

namespace bony
{
using namespace juce;
using namespace theme;

class SplitPage::Tape : public Component
{
public:
    Tape (BonyProcessor& p, int s, Colour c) : proc (p), slot (s), accent (c)
    {
        addAndMakeVisible (listen); listen.setColour (ToggleButton::tickColourId, c); listen.setToggleState (true, dontSendNotification);
        listen.onClick = [this] { proc.stemAudible[(size_t) slot] = listen.getToggleState(); };
        addAndMakeVisible (vol); vol.setSliderStyle (Slider::LinearHorizontal); vol.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        vol.setRange (0.0, 1.5, 0.01); vol.setValue (1.0); vol.setColour (Slider::thumbColourId, c);
        vol.onValueChange = [this] { proc.stemGain[(size_t) slot] = (float) vol.getValue(); };
        addAndMakeVisible (save); save.setColour (TextButton::buttonColourId, c);
        save.onClick = [this]
        {
            chooser = std::make_shared<FileChooser> ("Save stem", File::getSpecialLocation (File::userMusicDirectory).getChildFile (String (stemSlotName (slot)).toLowerCase() + ".wav"), "*.wav");
            chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this] (const FileChooser& fc)
            { auto r = fc.getResult(); if (r != File()) proc.saveStemWav (slot, r.withFileExtension ("wav")); });
        };
        setMouseCursor (MouseCursor::DraggingHandCursor);
    }
    bool present() const { auto s = proc.getStems(); return s && slot < (int) s->stems.size() && s->stems[(size_t) slot].present; }
    void paint (Graphics& g) override
    {
        const bool have = present();
        const bool audible = have && listen.getToggleState() && proc.getPlayMode() == PlayMode::Split;
        art::drawCassette (g, Rectangle<float> (((float) getWidth() - 160.0f) * 0.5f, 30.0f, 160.0f, 100.0f), stemSlotName (slot), accent, angle, have && listen.getToggleState(), ! have, slot == kLead);
        if (! have) { g.setColour (dim.brighter (0.4f)); g.setFont (display (14.0f)); g.drawText ("EMPTY - HIT SPLIT IT", 0, 134, getWidth(), 14, Justification::centred); }
        else { g.setColour (accent); g.setFont (body (11.0f, true)); g.drawText ("DRAG ME INTO YOUR DAW", 0, 134, getWidth(), 14, Justification::centred); }
        juce::ignoreUnused (audible);
    }
    void resized() override
    {
        auto r = getLocalBounds().withTrimmedTop (152).reduced (14, 0);
        listen.setBounds (r.removeFromLeft (84).withHeight (26)); r.removeFromLeft (4);
        save.setBounds (r.removeFromRight (56).withHeight (28)); r.removeFromRight (4);
        vol.setBounds (r.withHeight (26));
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() < 8 || dragging || ! present()) return;
        const auto f = proc.writeStemToTemp (slot);
        if (! f.existsAsFile()) return;
        dragging = true;
        if (auto* c = DragAndDropContainer::findParentDragContainerFor (this))
            c->performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
        else dragging = false;
    }
    float angle = 0.0f;
    ToggleButton listen { "LISTEN" };
private:
    BonyProcessor& proc; int slot; Colour accent; bool dragging = false;
    Slider vol; TextButton save { "SAVE" };
    std::shared_ptr<FileChooser> chooser;
};

SplitPage::SplitPage (BonyProcessor& p) : proc (p)
{
    const Colour cols[kNumSlots] = { pink, yellow, cyan, orange, lime, violet.brighter (0.4f) };
    for (int i = 0; i < kNumSlots; ++i) { tapes.add (new Tape (p, i, cols[i])); addAndMakeVisible (tapes.getLast()); }
    for (auto* b : { &dlBtn, &goBtn, &playBtn, &saveAllBtn, &folderBtn }) addAndMakeVisible (b);
    for (auto* t : { &optInstr, &optVocal, &optQuick }) addAndMakeVisible (t);
    optInstr.setToggleState (true, dontSendNotification); optVocal.setToggleState (true, dontSendNotification);
    optInstr.setColour (ToggleButton::tickColourId, orange); optVocal.setColour (ToggleButton::tickColourId, yellow); optQuick.setColour (ToggleButton::tickColourId, lime);
    dlBtn.setColour (TextButton::buttonColourId, orange);
    goBtn.setColour (TextButton::buttonColourId, pink);
    playBtn.setColour (TextButton::buttonColourId, yellow); playBtn.setColour (TextButton::buttonOnColourId, orange);
    saveAllBtn.setColour (TextButton::buttonColourId, cyan);
    folderBtn.setColour (TextButton::buttonColourId, violet.brighter (0.4f));

    dlBtn.onClick = [this] { proc.models.startDownload(); refresh(); };
    folderBtn.onClick = [] { auto d = ModelManager::modelsFolder(); d.createDirectory(); d.revealToUser(); };
    goBtn.onClick = [this]
    {
        if (proc.stems.getState() == StemEngine::State::Working) { proc.stems.cancel(); return; }
        if (! proc.getSource()) return;
        proc.setPlayMode (PlayMode::Off);
        StemEngine::Options o; o.instruments = optInstr.getToggleState(); o.vocalSplit = optVocal.getToggleState(); o.quickTest = optQuick.getToggleState();
        proc.stems.start (proc.getSource(), o);
    };
    playBtn.onClick = [this]
    {
        if (proc.getPlayMode() == PlayMode::Split) proc.setPlayMode (PlayMode::Off);
        else if (proc.getStems()) proc.setPlayMode (PlayMode::Split);
    };
    saveAllBtn.onClick = [this]
    {
        if (! proc.getStems()) return;
        chooser = std::make_shared<FileChooser> ("Pick a folder for the stems", File::getSpecialLocation (File::userMusicDirectory));
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories, [this] (const FileChooser& fc)
        {
            auto dir = fc.getResult(); if (! dir.isDirectory()) return;
            auto st = proc.getStems(); if (! st) return;
            auto base = File::createLegalFileName (st->sourceName.upToLastOccurrenceOf (".", false, false));
            for (int i = 0; i < kNumSlots; ++i) proc.saveStemWav (i, dir.getChildFile (base + " - " + String (stemSlotName (i)).toLowerCase() + ".wav"));
        });
    };
    proc.addChangeListener (this);
    startTimerHz (20);
    refresh();
}

SplitPage::~SplitPage() { proc.removeChangeListener (this); }

void SplitPage::timerCallback()
{
    const bool playing = proc.getPlayMode() == PlayMode::Split;
    const bool busy = proc.stems.getState() == StemEngine::State::Working || proc.models.isDownloading();
    if (playing || busy) { spin += 0.18f; for (auto* t : tapes) { if (playing && t->listen.getToggleState()) t->angle += 0.22f; t->repaint(); } }
    if (busy) refresh();
}

void SplitPage::refresh()
{
    const bool have = proc.models.allPresent();
    const bool working = proc.stems.getState() == StemEngine::State::Working;
    dlBtn.setVisible (! have);
    dlBtn.setEnabled (! proc.models.isDownloading());
    goBtn.setEnabled (have && (proc.getSource() != nullptr || working));
    goBtn.setButtonText (working ? "CANCEL" : "SPLIT IT!");
    playBtn.setButtonText (proc.getPlayMode() == PlayMode::Split ? "STOP" : "PLAY");
    playBtn.setToggleState (proc.getPlayMode() == PlayMode::Split, dontSendNotification);
    playBtn.setEnabled (proc.getStems() != nullptr);
    saveAllBtn.setEnabled (proc.getStems() != nullptr);

    if (proc.models.isDownloading()) { counterValue = proc.models.progress(); counterText = proc.models.statusText() + " " + String ((int) (counterValue * 100)) + "%"; }
    else if (working) { counterValue = proc.stems.progress(); counterText = proc.stems.statusText() + "  " + String ((int) (counterValue * 100)) + "%"; }
    else if (proc.stems.getState() == StemEngine::State::Failed) { counterValue = 0; counterText = proc.stems.errorText(); }
    else if (proc.models.lastError().isNotEmpty()) { counterValue = 0; counterText = proc.models.lastError(); }
    else if (! have) { counterValue = 0; counterText = "STEP 1: DOWNLOAD THE MODELS ONCE (ABOUT 170 MB)"; }
    else if (! proc.getSource()) { counterValue = 0; counterText = "LOAD A SONG ON THE GRAB PAGE (OR DROP ONE ON THE PLUGIN)"; }
    else if (proc.getStems()) { counterValue = 1.0f; counterText = "DONE - DRAG THE TAPES OUT"; }
    else { counterValue = 0; counterText = "READY: " + proc.getSource()->name; }
    repaint();
    for (auto* t : tapes) t->repaint();
}

void SplitPage::paint (Graphics& g)
{
    art::drawDeck (g, getLocalBounds().reduced (12, 8).toFloat().withTrimmedBottom (4), "STEM SPLITTER", cyan);
    art::drawTapeCounter (g, Rectangle<float> (26.0f, 58.0f, (float) getWidth() - 52.0f, 30.0f), counterValue, counterText.toUpperCase());
    g.setColour (dim.brighter (0.5f)); g.setFont (body (12.0f, true));
    g.drawText ("A full song takes a few minutes. QUICK TEST only does the first 30 seconds.", 330, getHeight() - 52, 400, 30, Justification::centred);
    art::drawStar (g, { (float) getWidth() - 40.0f, 30.0f }, 16.0f, yellow, ink, 0.3f);
}

void SplitPage::resized()
{
    auto r = getLocalBounds().reduced (26, 20);
    auto top = r.removeFromTop (34);
    optInstr.setBounds (top.removeFromLeft (190)); optVocal.setBounds (top.removeFromLeft (210)); optQuick.setBounds (top.removeFromLeft (170));
    goBtn.setBounds (top.removeFromRight (140)); top.removeFromRight (8);
    dlBtn.setBounds (top.removeFromRight (190));
    r.removeFromTop (40);          // tape counter
    auto bottom = r.removeFromBottom (44);
    playBtn.setBounds (bottom.removeFromLeft (120)); bottom.removeFromLeft (8);
    saveAllBtn.setBounds (bottom.removeFromLeft (130));
    folderBtn.setBounds (bottom.removeFromRight (170));
    r.removeFromBottom (24);
    const int cw = r.getWidth() / 3, ch = r.getHeight() / 2;
    for (int i = 0; i < kNumSlots; ++i)
        tapes[i]->setBounds (r.getX() + (i % 3) * cw, r.getY() + (i / 3) * ch, cw, ch);
}
}
