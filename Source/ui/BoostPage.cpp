#include "BoostPage.h"
#include "Art.h"

namespace bony
{
using namespace juce;
using namespace theme;

BoostPage::BoostPage (BonyProcessor& p) : proc (p)
{
    amount.setSliderStyle (Slider::RotaryHorizontalVerticalDrag); amount.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    amount.setRotaryParameters (MathConstants<float>::pi * 1.15f, MathConstants<float>::pi * 2.85f, true);
    amount.setColour (Slider::rotarySliderFillColourId, pink);
    addAndMakeVisible (amount);
    outTrim.setSliderStyle (Slider::RotaryHorizontalVerticalDrag); outTrim.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    outTrim.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    outTrim.setColour (Slider::rotarySliderFillColourId, cyan);
    addAndMakeVisible (outTrim);
    amountA = std::make_unique<SA> (proc.apvts, ids::boostAmt, amount);
    outA = std::make_unique<SA> (proc.apvts, ids::boostOut, outTrim);

    const Colour mc[4] = { cyan, lime, yellow, orange };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = modeBtn[i];
        b.setButtonText (modeNames[i]); b.setClickingTogglesState (false);
        b.setColour (TextButton::buttonColourId, mc[i].darker (0.5f)); b.setColour (TextButton::buttonOnColourId, mc[i]);
        b.setColour (TextButton::textColourOffId, cream); b.setColour (TextButton::textColourOnId, ink);
        b.onClick = [this, i] { if (auto* prm = proc.apvts.getParameter (ids::boostMode)) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i)); prm->endChangeGesture(); } };
        addAndMakeVisible (b);
    }
    powerBtn.setClickingTogglesState (false);
    powerBtn.setColour (TextButton::buttonColourId, Colour (0xff4a2a5c)); powerBtn.setColour (TextButton::buttonOnColourId, lime);
    powerBtn.setColour (TextButton::textColourOffId, cream); powerBtn.setColour (TextButton::textColourOnId, ink);
    powerBtn.onClick = [this]
    {
        if (auto* prm = proc.apvts.getParameter (ids::boostOn)) { const bool now = prm->getValue() < 0.5f; prm->beginChangeGesture(); prm->setValueNotifyingHost (now ? 1.0f : 0.0f); prm->endChangeGesture(); }
    };
    addAndMakeVisible (powerBtn);
    lOut.setText ("OUTPUT", dontSendNotification); lOut.setJustificationType (Justification::centred); lOut.setFont (display (16.0f));
    addAndMakeVisible (lOut);
    startTimerHz (30);
}

void BoostPage::timerCallback()
{
    phase += 0.12f;
    const int mode = (int) proc.apvts.getRawParameterValue (ids::boostMode)->load();
    for (int i = 0; i < 4; ++i) modeBtn[i].setToggleState (i == mode, dontSendNotification);
    const bool on = proc.apvts.getRawParameterValue (ids::boostOn)->load() > 0.5f;
    powerBtn.setToggleState (on, dontSendNotification);
    powerBtn.setButtonText (on ? "POWER ON" : "POWER OFF");
    repaint();
}

void BoostPage::drawMeter (Graphics& g, Rectangle<float> r, float v, const String& label, bool reversed)
{
    g.setColour (ink); g.fillRoundedRectangle (r, 6.0f);
    const int segs = 16;
    auto inner = r.reduced (5.0f);
    const float sh = inner.getHeight() / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const float t = (float) i / (float) segs;
        const bool lit = reversed ? (t < v) : (t < v);
        const float y = reversed ? inner.getY() + sh * (float) i : inner.getBottom() - sh * (float) (i + 1);
        Colour c = reversed ? cyan : (t > 0.8f ? pink : t > 0.6f ? yellow : lime);
        g.setColour (lit ? c : c.withAlpha (0.12f));
        g.fillRoundedRectangle (inner.getX(), y + 1.0f, inner.getWidth(), sh - 2.0f, 2.0f);
    }
    g.setColour (cream); g.setFont (display (14.0f));
    g.drawText (label, r.translated (0.0f, r.getHeight() + 2.0f).withHeight (18.0f), Justification::centred);
}

void BoostPage::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (12.0f, 8.0f).withTrimmedBottom (4.0f);
    art::drawDeck (g, b, "BONY BOOST", pink);

    const float pulse = std::sin (phase) * std::min (1.0f, proc.outLevel.load() * 3.0f);
    const bool on = proc.apvts.getRawParameterValue (ids::boostOn)->load() > 0.5f;
    art::drawSpeaker (g, Rectangle<float> (40.0f, b.getBottom() - 170.0f, 150.0f, 150.0f), on ? pulse : 0.0f);
    art::drawSpeaker (g, Rectangle<float> (b.getRight() - 190.0f, b.getBottom() - 170.0f, 150.0f, 150.0f), on ? -pulse : 0.0f);

    // big knob with crown
    auto k = knobArea;
    g.setColour (ink.withAlpha (0.6f)); g.fillEllipse (k.expanded (14.0f));
    art::drawCrown (g, Rectangle<float> (k.getCentreX() - 56.0f, k.getY() - 70.0f, 112.0f, 86.0f), on);
    g.setColour (on ? pink : dim); g.setFont (display (34.0f));
    g.drawText ("BOOST  " + String ((int) amount.getValue()) + "%", Rectangle<float> (k.getX() - 40.0f, k.getBottom() + 8.0f, k.getWidth() + 80.0f, 40.0f), Justification::centred);
    g.setColour (cream.withAlpha (0.8f)); g.setFont (body (13.0f, true));
    g.drawText ("One knob: multiband squeeze + warmth + limiter. Pick a flavour, turn it up till it hits.",
                Rectangle<float> (b.getX() + 40.0f, k.getBottom() + 46.0f, b.getWidth() - 80.0f, 18.0f), Justification::centred);

    // meters
    smoothIn = jmax (proc.inLevel.load(), smoothIn * 0.85f);
    smoothOut = jmax (proc.outLevel.load(), smoothOut * 0.85f);
    smoothGr = jmax (proc.boost.grMeter.load(), smoothGr * 0.9f);
    auto toMeter = [] (float lin) { return jlimit (0.0f, 1.0f, (Decibels::gainToDecibels (lin + 1.0e-6f) + 48.0f) / 48.0f); };
    drawMeter (g, Rectangle<float> (b.getX() + 40.0f, b.getY() + 40.0f, 26.0f, 150.0f), toMeter (smoothIn), "IN", false);
    drawMeter (g, Rectangle<float> (b.getX() + 80.0f, b.getY() + 40.0f, 26.0f, 150.0f), jlimit (0.0f, 1.0f, smoothGr * 1.2f), "SQZ", true);
    drawMeter (g, Rectangle<float> (b.getRight() - 66.0f, b.getY() + 40.0f, 26.0f, 150.0f), toMeter (smoothOut), "OUT", false);

    art::drawSticker (g, "LOUD & PROUD", Rectangle<float> (b.getRight() - 250.0f, b.getY() + 24.0f, 160.0f, 28.0f), yellow, ink, 4.0f);
    art::drawStar (g, { b.getX() + 150.0f, b.getY() + 30.0f }, 14.0f, cyan, ink, 0.2f);
    art::drawBolt (g, Rectangle<float> (b.getCentreX() - 190.0f, b.getBottom() - 74.0f, 26.0f, 38.0f), yellow);
    art::drawBolt (g, Rectangle<float> (b.getCentreX() + 164.0f, b.getBottom() - 74.0f, 26.0f, 38.0f), cyan);
}

void BoostPage::resized()
{
    auto b = getLocalBounds().reduced (12, 8);
    const float s = 190.0f;
    knobArea = Rectangle<float> ((float) b.getCentreX() - s * 0.5f, (float) b.getY() + 142.0f, s, s);
    amount.setBounds (knobArea.toNearestInt());
    for (int i = 0; i < 4; ++i)
        modeBtn[i].setBounds (b.getCentreX() - 130 + i * 66, b.getY() + 20, 60, 44);
    powerBtn.setBounds (b.getCentreX() - 70, b.getBottom() - 70, 140, 50);
    outTrim.setBounds (b.getRight() - 170, b.getY() + 215, 90, 90);
    lOut.setBounds (b.getRight() - 180, b.getY() + 195, 110, 20);
}
}
