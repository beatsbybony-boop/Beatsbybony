#include "BonyLookAndFeel.h"
#include "Art.h"
#include "Tag.h"

namespace bony
{
using namespace juce;
using namespace theme;

BonyLookAndFeel::BonyLookAndFeel()
{
    setColour (Label::textColourId, cream);
    setColour (ComboBox::backgroundColourId, ink);
    setColour (ComboBox::textColourId, cream);
    setColour (ComboBox::outlineColourId, pink);
    setColour (ComboBox::arrowColourId, yellow);
    setColour (PopupMenu::backgroundColourId, bg1);
    setColour (PopupMenu::textColourId, cream);
    setColour (PopupMenu::highlightedBackgroundColourId, pink);
    setColour (PopupMenu::highlightedTextColourId, ink);
    setColour (TextButton::buttonColourId, yellow);
    setColour (TextButton::textColourOffId, ink);
    setColour (TextButton::textColourOnId, ink);
    setColour (TooltipWindow::backgroundColourId, ink);
    setColour (TooltipWindow::textColourId, cream);
}

void BonyLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float s, float e, Slider& sl)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    auto c = b.getCentre();
    const auto accent = sl.findColour (Slider::rotarySliderFillColourId);
    const float ang = s + pos * (e - s);

    // glow arc
    Path arc; arc.addCentredArc (c.x, c.y, r, r, 0.0f, s, e, true);
    g.setColour (ink); g.strokePath (arc, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
    Path val; val.addCentredArc (c.x, c.y, r, r, 0.0f, s, ang, true);
    for (int i = 3; i >= 1; --i) { g.setColour (accent.withAlpha (0.10f * (float) (4 - i))); g.strokePath (val, PathStrokeType (3.0f + (float) i * 2.5f, PathStrokeType::curved, PathStrokeType::rounded)); }
    g.setColour (accent); g.strokePath (val, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    // gold ring + chrome body
    const float rr = r - 9.0f;
    g.setColour (ink); g.fillEllipse (c.x - rr - 3, c.y - rr - 2, (rr + 3) * 2, (rr + 3) * 2);
    g.setGradientFill (ColourGradient (Colour (0xffffe27a), c.x - rr, c.y - rr, goldDark, c.x + rr, c.y + rr, false));
    g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
    const float br = rr * 0.80f;
    g.setGradientFill (ColourGradient (Colour (0xff6d6680), c.x - br * 0.6f, c.y - br * 0.7f, Colour (0xff16121f), c.x + br, c.y + br, true));
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (ink); g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.5f);

    Path p; p.addRoundedRectangle (-2.5f, -br + 3.0f, 5.0f, br * 0.55f, 2.0f);
    g.setColour (ink);   g.fillPath (p, AffineTransform::rotation (ang).translated (c.x + 1.0f, c.y + 1.0f));
    g.setColour (accent.brighter (0.3f)); g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
}

void BonyLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle st, Slider& sl)
{
    const auto accent = sl.findColour (Slider::thumbColourId);
    const bool vertical = st == Slider::LinearVertical;
    Rectangle<float> track = vertical ? Rectangle<float> ((float) x + w * 0.5f - 6.0f, (float) y + 6.0f, 12.0f, (float) h - 12.0f)
                                      : Rectangle<float> ((float) x + 6.0f, (float) y + h * 0.5f - 6.0f, (float) w - 12.0f, 12.0f);
    g.setColour (ink); g.fillRoundedRectangle (track.expanded (2.0f), 8.0f);
    g.setColour (Colour (0xff0f0818)); g.fillRoundedRectangle (track, 6.0f);

    Rectangle<float> fill = vertical ? track.withTop (pos) : track.withRight (pos);
    for (int i = 3; i >= 1; --i) { g.setColour (accent.withAlpha (0.10f * (float) (4 - i))); g.fillRoundedRectangle (fill.expanded ((float) i), 6.0f); }
    g.setGradientFill (vertical ? ColourGradient::vertical (accent.brighter (0.3f), fill.getY(), accent.darker (0.4f), fill.getBottom())
                                : ColourGradient::horizontal (accent.darker (0.4f), fill.getX(), accent.brighter (0.3f), fill.getRight()));
    g.fillRoundedRectangle (fill, 6.0f);

    // fader cap
    Rectangle<float> cap = vertical ? Rectangle<float> (track.getCentreX() - 17.0f, pos - 9.0f, 34.0f, 18.0f)
                                    : Rectangle<float> (pos - 9.0f, track.getCentreY() - 17.0f, 18.0f, 34.0f);
    g.setColour (ink); g.fillRoundedRectangle (cap.translated (2.0f, 3.0f), 4.0f);
    g.setGradientFill (ColourGradient::vertical (Colour (0xfff0eaf8), cap.getY(), Colour (0xff8f879f), cap.getBottom()));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (ink); g.drawRoundedRectangle (cap, 4.0f, 2.0f);
    g.setColour (accent);
    if (vertical) g.fillRect (cap.getX() + 4.0f, cap.getCentreY() - 1.5f, cap.getWidth() - 8.0f, 3.0f);
    else          g.fillRect (cap.getCentreX() - 1.5f, cap.getY() + 4.0f, 3.0f, cap.getHeight() - 8.0f);
}

void BonyLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = b.getToggleState();
    auto col = on ? b.findColour (TextButton::buttonOnColourId) : base;
    const float off = down ? 1.0f : 0.0f;
    if (! down) { g.setColour (ink); g.fillRoundedRectangle (r.translated (3.0f, 3.5f), 7.0f); }
    r = r.translated (off * 2.0f, off * 2.0f);
    g.setColour (Colours::white); g.fillRoundedRectangle (r.expanded (2.0f), 8.0f);
    g.setGradientFill (ColourGradient::vertical (col.brighter (over ? 0.5f : 0.3f), r.getY(), col.darker (0.25f), r.getBottom()));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (ink); g.drawRoundedRectangle (r, 6.0f, 2.0f);
}

void BonyLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool down)
{
    const auto col = b.getToggleState() ? b.findColour (TextButton::textColourOnId) : b.findColour (TextButton::textColourOffId);
    auto r = b.getLocalBounds().reduced (10, 8).toFloat();
    if (down) r.translate (2.0f, 2.0f);
    const bool dark = col.getBrightness() > 0.6f;
    if (b.getHeight() >= 30)   // big buttons get real graffiti lettering
    {
        tag::GStyle gs; gs.fillTop = col.brighter (0.35f); gs.fillBottom = col; gs.outline = ink; gs.inlineCol = ink.withAlpha (0.0f);
        gs.extrudeTop = ink.withAlpha (0.55f); gs.extrudeBottom = ink.withAlpha (0.55f);
        gs.thickness = 0.2f; gs.outlineW = 0.06f; gs.inlineW = 0.0f; gs.depth = 0.05f; gs.overlap = 0.0f; gs.slant = 0.22f; gs.jitter = 0.5f;
        gs.spikes = false; gs.drips = false; gs.glints = false; gs.spray = false; gs.seed = b.getButtonText().length();
        tag::drawGraffiti (g, b.getButtonText(), r.expanded (4.0f, 2.0f), gs);
        return;
    }
    tag::drawSimple (g, b.getButtonText(), r, col, dark ? ink : Colours::transparentBlack, dark ? 0.17f : 0.15f);
}

void BonyLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.0f);
    auto box = r.removeFromLeft (r.getHeight()).reduced (2.0f);
    g.setColour (ink); g.fillRoundedRectangle (box.translated (2, 2), 4.0f);
    g.setColour (Colours::white); g.fillRoundedRectangle (box.expanded (1.5f), 5.0f);
    g.setColour (b.getToggleState() ? b.findColour (ToggleButton::tickColourId) : Colour (0xff2a2038));
    g.fillRoundedRectangle (box, 3.5f);
    g.setColour (ink); g.drawRoundedRectangle (box, 3.5f, 1.5f);
    if (b.getToggleState())
    {
        Path t; t.startNewSubPath (box.getX() + box.getWidth() * 0.22f, box.getCentreY()); t.lineTo (box.getX() + box.getWidth() * 0.43f, box.getBottom() - box.getHeight() * 0.25f); t.lineTo (box.getRight() - box.getWidth() * 0.2f, box.getY() + box.getHeight() * 0.22f);
        g.strokePath (t, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (cream); g.setFont (body (14.0f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (6.0f), Justification::centredLeft);
}

void BonyLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool down, int, int, int, int, ComboBox& cb)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (2.0f);
    g.setColour (ink); g.fillRoundedRectangle (r.translated (2, 2.5f), 6.0f);
    g.setColour (down ? bg1.brighter (0.2f) : Colour (0xff120a20)); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (cb.findColour (ComboBox::outlineColourId)); g.drawRoundedRectangle (r, 6.0f, 2.0f);
    Path a; const float cx = (float) w - 16.0f, cy = (float) h * 0.5f;
    a.addTriangle (cx - 5, cy - 3, cx + 5, cy - 3, cx, cy + 4);
    g.setColour (yellow); g.fillPath (a);
}
}
