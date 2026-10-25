#include "Art.h"
#include <cmath>

namespace bony::theme
{
static bool hasTypeface (const juce::String& name)
{
    static const juce::StringArray all = juce::Font::findAllTypefaceNames();
    return all.contains (name);
}

juce::Font display (float h)
{
    for (auto* n : { "Impact", "Haettenschweiler", "Arial Black", "DejaVu Sans" })
        if (hasTypeface (n)) return juce::Font (juce::FontOptions (n, h, juce::Font::plain));
    return juce::Font (juce::FontOptions (h, juce::Font::bold));
}

juce::Font body (float h, bool bold)
{
    for (auto* n : { "Arial Narrow", "Bahnschrift", "Segoe UI", "DejaVu Sans" })
        if (hasTypeface (n)) return juce::Font (juce::FontOptions (n, h, bold ? juce::Font::bold : juce::Font::plain));
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}
}

namespace bony::art
{
using namespace juce;
using namespace bony::theme;

static Path textPath (const Font& f, const String& t, Rectangle<float> area, Justification j)
{
    GlyphArrangement ga;
    ga.addFittedText (f, t, area.getX(), area.getY(), area.getWidth(), area.getHeight(), j, 1, 0.6f);
    Path p;
    ga.createPath (p);
    return p;
}

// ------------------------------------------------------------------ bricks
void drawBricks (Graphics& g, Rectangle<int> area, int seed)
{
    g.setColour (bg0);
    g.fillRect (area);
    Random r (seed);
    const int bh = 26, bw = 64;
    for (int row = 0, y = area.getY(); y < area.getBottom(); ++row, y += bh)
    {
        const int off = (row % 2) ? -bw / 2 : 0;
        for (int x = area.getX() + off; x < area.getRight(); x += bw)
        {
            const float v = r.nextFloat();
            auto c = Colour (0xff2b1238).interpolatedWith (Colour (0xff45204f), v * 0.9f);
            if (r.nextInt (11) == 0) c = c.interpolatedWith (pink, 0.12f);
            if (r.nextInt (13) == 0) c = c.interpolatedWith (cyan, 0.10f);
            auto b = Rectangle<float> ((float) x + 1.5f, (float) y + 1.5f, (float) bw - 3.0f, (float) bh - 3.0f);
            g.setColour (c);
            g.fillRoundedRectangle (b, 2.0f);
            g.setColour (Colours::white.withAlpha (0.045f));
            g.fillRect (b.removeFromTop (2.0f));
            g.setColour (Colours::black.withAlpha (0.25f));
            g.fillRect (b.removeFromBottom (3.0f));
        }
    }
    // darken toward the edges so the content pops
    ColourGradient vg (Colours::transparentBlack, area.getCentreX(), area.getCentreY(),
                       Colours::black.withAlpha (0.55f), (float) area.getX(), (float) area.getY(), true);
    g.setGradientFill (vg);
    g.fillRect (area);
}

// ------------------------------------------------------------------ crown
void drawCrown (Graphics& g, Rectangle<float> a, bool glow)
{
    const float w = a.getWidth(), h = a.getHeight();
    auto P = [&] (float x, float y) { return Point<float> (a.getX() + x * w, a.getY() + y * h); };

    Path crown;
    crown.startNewSubPath (P (0.06f, 0.88f));
    crown.lineTo (P (0.02f, 0.26f));
    crown.lineTo (P (0.27f, 0.56f));
    crown.lineTo (P (0.50f, 0.06f));
    crown.lineTo (P (0.73f, 0.56f));
    crown.lineTo (P (0.98f, 0.26f));
    crown.lineTo (P (0.94f, 0.88f));
    crown.closeSubPath();
    crown = crown.createPathWithRoundedCorners (h * 0.07f);

    if (glow)
    {
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (gold.withAlpha (0.07f * (float) (4 - i)));
            g.strokePath (crown, PathStrokeType ((float) i * h * 0.12f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }
    g.setColour (ink);
    g.strokePath (crown, PathStrokeType (h * 0.12f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setGradientFill (ColourGradient::vertical (Colour (0xffffe27a), a.getY(), goldDark, a.getBottom()));
    g.fillPath (crown);

    // band
    Rectangle<float> band (a.getX() + w * 0.055f, a.getY() + h * 0.70f, w * 0.89f, h * 0.18f);
    g.setColour (goldDark.darker (0.3f));
    g.fillRoundedRectangle (band, h * 0.04f);
    g.setColour (ink);
    g.drawRoundedRectangle (band, h * 0.04f, h * 0.03f);

    // jewels
    auto jewel = [&] (Point<float> c, float r, Colour col)
    {
        g.setColour (ink);   g.fillEllipse (c.x - r * 1.25f, c.y - r * 1.25f, r * 2.5f, r * 2.5f);
        g.setColour (col);   g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (Colours::white.withAlpha (0.8f)); g.fillEllipse (c.x - r * 0.5f, c.y - r * 0.6f, r * 0.55f, r * 0.55f);
    };
    jewel (P (0.02f, 0.24f), h * 0.075f, pink);
    jewel (P (0.50f, 0.05f), h * 0.085f, cyan);
    jewel (P (0.98f, 0.24f), h * 0.075f, pink);
    jewel (P (0.25f, 0.79f), h * 0.055f, cyan);
    jewel (P (0.50f, 0.79f), h * 0.055f, pink);
    jewel (P (0.75f, 0.79f), h * 0.055f, cyan);

    // shine
    g.setColour (Colours::white.withAlpha (0.45f));
    Path shine; shine.startNewSubPath (P (0.12f, 0.62f)); shine.lineTo (P (0.10f, 0.40f));
    g.strokePath (shine, PathStrokeType (h * 0.04f, PathStrokeType::curved, PathStrokeType::rounded));
}

// ------------------------------------------------------------------ cassette
static void drawReel (Graphics& g, Point<float> c, float r, float angle)
{
    g.setColour (Colour (0xff15101c)); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colour (0xffe9e4f2)); g.fillEllipse (c.x - r * 0.62f, c.y - r * 0.62f, r * 1.24f, r * 1.24f);
    g.setColour (Colour (0xff15101c));
    g.fillEllipse (c.x - r * 0.34f, c.y - r * 0.34f, r * 0.68f, r * 0.68f);
    g.setColour (Colour (0xffe9e4f2));
    for (int i = 0; i < 6; ++i)
    {
        const float a = angle + (float) i * MathConstants<float>::pi / 3.0f;
        Path tooth; tooth.addRectangle (-r * 0.05f, -r * 0.40f, r * 0.10f, r * 0.17f);
        g.fillPath (tooth, AffineTransform::rotation (a).translated (c.x, c.y));
    }
}

void drawCassette (Graphics& g, Rectangle<float> a, const String& label, Colour accent, float angle, bool lit, bool empty, bool withCrown)
{
    Graphics::ScopedSaveState ss (g);
    if (empty) g.setOpacity (0.38f);
    const float w = a.getWidth(), h = a.getHeight();

    if (lit)
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (accent.withAlpha (0.05f * (float) (5 - i)));
            g.fillRoundedRectangle (a.expanded ((float) i * 3.0f), h * 0.1f + (float) i * 3.0f);
        }

    g.setColour (ink); g.fillRoundedRectangle (a.translated (3.0f, 4.0f), h * 0.1f);                 // hard drop shadow
    g.setGradientFill (ColourGradient::vertical (Colour (0xff3b3548), a.getY(), Colour (0xff1c1826), a.getBottom()));
    g.fillRoundedRectangle (a, h * 0.1f);
    g.setColour (ink); g.drawRoundedRectangle (a, h * 0.1f, 2.5f);
    g.setColour (Colours::white.withAlpha (0.13f)); g.drawRoundedRectangle (a.reduced (2.5f), h * 0.09f, 1.0f);

    // label sticker
    auto lab = Rectangle<float> (a.getX() + w * 0.07f, a.getY() + h * 0.09f, w * 0.86f, h * 0.50f);
    g.setColour (cream); g.fillRoundedRectangle (lab, h * 0.04f);
    g.setColour (accent); g.fillRoundedRectangle (lab.withHeight (lab.getHeight() * 0.30f), h * 0.04f);
    g.fillRect (lab.getX(), lab.getY() + lab.getHeight() * 0.15f, lab.getWidth(), lab.getHeight() * 0.15f);
    g.setColour (ink); g.drawRoundedRectangle (lab, h * 0.04f, 1.6f);
    g.setColour (ink.withAlpha (0.35f));
    for (int i = 0; i < 2; ++i)
        g.drawLine (lab.getX() + w * 0.04f, lab.getBottom() - h * (0.05f + 0.045f * (float) i), lab.getRight() - w * 0.04f, lab.getBottom() - h * (0.05f + 0.045f * (float) i), 1.0f);

    g.setColour (ink);
    auto textArea = lab.withTrimmedTop (lab.getHeight() * 0.28f).reduced (w * 0.03f, 0.0f);
    g.setFont (display (jmin (h * 0.19f, 26.0f)));
    g.drawFittedText (label, textArea.toNearestInt().withTrimmedBottom ((int) (h * 0.06f)), Justification::centred, 1, 0.55f);

    // tape window with reels
    auto win = Rectangle<float> (a.getX() + w * 0.20f, a.getY() + h * 0.62f, w * 0.60f, h * 0.24f);
    g.setColour (Colour (0xff0e0a14)); g.fillRoundedRectangle (win, win.getHeight() * 0.5f);
    g.setColour (Colour (0xff6b5a7d)); g.drawRoundedRectangle (win, win.getHeight() * 0.5f, 1.5f);
    const float rr = win.getHeight() * 0.44f;
    drawReel (g, { win.getX() + rr * 1.25f, win.getCentreY() }, rr, angle);
    drawReel (g, { win.getRight() - rr * 1.25f, win.getCentreY() }, rr, -angle);
    g.setColour (Colour (0xff4a3426));
    g.fillRect (win.getX() + rr * 1.25f + rr * 0.6f, win.getCentreY() - 1.0f, win.getWidth() - rr * 2.5f - rr * 1.2f, 2.0f);

    // screws
    g.setColour (Colour (0xff8d86a0));
    for (auto p : { Point<float> (a.getX() + w * 0.045f, a.getY() + h * 0.07f), Point<float> (a.getRight() - w * 0.045f, a.getY() + h * 0.07f),
                    Point<float> (a.getX() + w * 0.045f, a.getBottom() - h * 0.07f), Point<float> (a.getRight() - w * 0.045f, a.getBottom() - h * 0.07f) })
        g.fillEllipse (p.x - 2.5f, p.y - 2.5f, 5.0f, 5.0f);

    if (withCrown)
        drawCrown (g, Rectangle<float> (a.getCentreX() - w * 0.17f, a.getY() - h * 0.34f, w * 0.34f, h * 0.38f), lit);
}

// ------------------------------------------------------------------ graffiti text
static void drips (Graphics& g, const Path& p, Colour c, int seed)
{
    auto b = p.getBounds();
    Random r (seed);
    const int count = jmax (3, (int) (b.getWidth() / 34.0f));
    for (int i = 0; i < count; ++i)
    {
        const float x = b.getX() + b.getWidth() * (0.06f + 0.88f * r.nextFloat());
        const float len = b.getHeight() * (0.10f + 0.34f * r.nextFloat());
        const float wd = 2.6f + 2.0f * r.nextFloat();
        const float y0 = b.getBottom() - b.getHeight() * 0.06f;
        g.setColour (ink);
        g.fillRoundedRectangle (x - wd * 0.5f - 1.5f, y0, wd + 3.0f, len + 1.5f, wd * 0.5f);
        g.fillEllipse (x - wd * 0.9f - 1.5f, y0 + len - wd * 0.6f, wd * 1.8f + 3.0f, wd * 1.8f + 3.0f);
        g.setColour (c);
        g.fillRoundedRectangle (x - wd * 0.5f, y0, wd, len, wd * 0.5f);
        g.fillEllipse (x - wd * 0.9f, y0 + len - wd * 0.5f, wd * 1.8f, wd * 1.8f);
    }
}

void drawGraffitiText (Graphics& g, const String& text, Rectangle<float> area, const GraffitiStyle& s)
{
    auto font = display (area.getHeight() * 0.92f);
    Path base = textPath (font, text, area, Justification::centred);
    if (base.isEmpty()) return;
    auto bb = base.getBounds();
    const float cx = bb.getCentreX(), by = bb.getBottom();
    AffineTransform t = AffineTransform::translation (-cx, -by).followedBy (AffineTransform::shear (-s.slant, 0.0f))
                            .followedBy (AffineTransform::rotation (degreesToRadians (s.rotationDeg)))
                            .followedBy (AffineTransform::translation (cx, by));
    base.applyTransform (t);

    Graphics::ScopedSaveState ss (g);
    auto stroke = [] (float w) { return PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded); };

    // hard drop shadow
    Path sh (base); sh.applyTransform (AffineTransform::translation (s.shadowDx, s.shadowDy));
    g.setColour (s.shadow);
    g.strokePath (sh, stroke (s.outlineWidth + s.rimWidth * 2.0f)); g.fillPath (sh);
    // outer outline, rim, fill
    g.setColour (s.outline); g.strokePath (base, stroke (s.outlineWidth + s.rimWidth * 2.0f));
    g.setColour (s.rim);     g.strokePath (base, stroke (s.rimWidth * 2.0f));
    auto b = base.getBounds();
    g.setGradientFill (ColourGradient::vertical (s.fillTop, b.getY(), s.fillBottom, b.getBottom()));
    g.fillPath (base);

    // gloss on the top half
    {
        Graphics::ScopedSaveState s2 (g);
        g.reduceClipRegion (Rectangle<int> ((int) b.getX() - 4, (int) b.getY() - 4, (int) b.getWidth() + 8, (int) (b.getHeight() * 0.34f)));
        g.setColour (Colours::white.withAlpha (0.22f));
        g.fillPath (base);
    }
    // glints
    drawStar (g, { b.getX() + b.getWidth() * 0.08f, b.getY() + b.getHeight() * 0.12f }, b.getHeight() * 0.16f, Colours::white, Colours::transparentBlack, 0.0f, 4);
    drawStar (g, { b.getRight() - b.getWidth() * 0.05f, b.getY() + b.getHeight() * 0.30f }, b.getHeight() * 0.11f, Colours::white, Colours::transparentBlack, 0.4f, 4);

    if (s.drips) drips (g, base, s.fillBottom, s.seed);

    // spray speckle
    Random r (s.seed * 31);
    g.setColour (s.fillBottom.withAlpha (0.55f));
    for (int i = 0; i < 46; ++i)
    {
        const float x = b.getX() - 6.0f + (b.getWidth() + 12.0f) * r.nextFloat();
        const float y = b.getY() - 6.0f + (b.getHeight() + 12.0f) * r.nextFloat();
        g.fillEllipse (x, y, 1.2f + 1.6f * r.nextFloat(), 1.2f + 1.6f * r.nextFloat());
    }
}

void drawLogo (Graphics& g, Rectangle<float> a)
{
    // "Beats" (pink)  "By" (cyan, small)  "Bony" (yellow) + crown
    const float h = a.getHeight();
    auto beats = Rectangle<float> (a.getX(), a.getY() + h * 0.10f, a.getWidth() * 0.38f, h * 0.80f);
    auto by    = Rectangle<float> (beats.getRight() + 4.0f, a.getY() + h * 0.42f, a.getWidth() * 0.11f, h * 0.36f);
    auto bony  = Rectangle<float> (by.getRight() + 4.0f, a.getY() + h * 0.10f, a.getWidth() * 0.47f, h * 0.80f);

    GraffitiStyle s1; s1.fillTop = Colour (0xffff7dc0); s1.fillBottom = pink; s1.outline = ink; s1.rim = Colours::white;
    s1.shadow = violet.darker (0.3f); s1.rotationDeg = -3.0f; s1.seed = 3; s1.outlineWidth = 5.0f; s1.rimWidth = 2.2f;
    GraffitiStyle s2 = s1; s2.fillTop = Colour (0xff9af3ff); s2.fillBottom = cyan; s2.drips = false; s2.rotationDeg = -6.0f; s2.shadowDx = 3; s2.shadowDy = 4; s2.seed = 5; s2.outlineWidth = 4.0f; s2.rimWidth = 1.8f;
    GraffitiStyle s3 = s1; s3.fillTop = Colour (0xffffef7a); s3.fillBottom = Colour (0xffffa800); s3.rotationDeg = -2.0f; s3.shadow = Colour (0xff6a1fd0); s3.seed = 11;

    drawGraffitiText (g, "Beats", beats, s1);
    drawGraffitiText (g, "By", by, s2);
    drawGraffitiText (g, "Bony", bony, s3);

    // crown sitting on the B of Bony
    drawCrown (g, Rectangle<float> (bony.getCentreX() - h * 0.34f, a.getY() - h * 0.26f, h * 0.68f, h * 0.52f), true);
    // underline swoosh
    Path sw;
    sw.startNewSubPath (a.getX() + 6.0f, a.getBottom() - 2.0f);
    sw.quadraticTo (a.getCentreX(), a.getBottom () + 8.0f, a.getRight() - 4.0f, a.getBottom() - 8.0f);
    g.setColour (ink);   g.strokePath (sw, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (lime);  g.strokePath (sw, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
}

// ------------------------------------------------------------------ small decorations
void drawSplat (Graphics& g, Point<float> c, float radius, Colour col, int seed)
{
    Random r (seed);
    g.setColour (col);
    g.fillEllipse (c.x - radius * 0.62f, c.y - radius * 0.62f, radius * 1.24f, radius * 1.24f);
    for (int i = 0; i < 9; ++i)
    {
        const float a = r.nextFloat() * MathConstants<float>::twoPi;
        const float d = radius * (0.5f + 0.55f * r.nextFloat());
        const float rr = radius * (0.10f + 0.20f * r.nextFloat());
        g.fillEllipse (c.x + std::cos (a) * d - rr, c.y + std::sin (a) * d - rr, rr * 2, rr * 2);
        Path spike; spike.startNewSubPath (c.x, c.y); spike.lineTo (c.x + std::cos (a) * d, c.y + std::sin (a) * d);
        g.strokePath (spike, PathStrokeType (rr * 1.1f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    for (int i = 0; i < 24; ++i)
    {
        const float a = r.nextFloat() * MathConstants<float>::twoPi;
        const float d = radius * (1.1f + 0.7f * r.nextFloat());
        g.fillEllipse (c.x + std::cos (a) * d, c.y + std::sin (a) * d, 1.5f + 2.0f * r.nextFloat(), 1.5f + 2.0f * r.nextFloat());
    }
}

void drawStar (Graphics& g, Point<float> c, float r, Colour fill, Colour outline, float rot, int pts)
{
    Path p;
    p.addStar (c, pts, r * (pts == 4 ? 0.18f : 0.45f), r, rot);
    if (outline.getAlpha() > 0) { g.setColour (outline); g.strokePath (p, PathStrokeType (2.5f, PathStrokeType::mitered)); }
    g.setColour (fill); g.fillPath (p);
}

void drawBolt (Graphics& g, Rectangle<float> a, Colour fill)
{
    auto P = [&] (float x, float y) { return Point<float> (a.getX() + x * a.getWidth(), a.getY() + y * a.getHeight()); };
    Path p;
    p.startNewSubPath (P (0.55f, 0.0f)); p.lineTo (P (0.10f, 0.58f)); p.lineTo (P (0.44f, 0.58f));
    p.lineTo (P (0.30f, 1.0f)); p.lineTo (P (0.92f, 0.38f)); p.lineTo (P (0.56f, 0.38f)); p.closeSubPath();
    g.setColour (ink); g.strokePath (p, PathStrokeType (4.0f, PathStrokeType::mitered));
    g.setColour (fill); g.fillPath (p);
}

void drawSticker (Graphics& g, const String& text, Rectangle<float> a, Colour fill, Colour textCol, float rot)
{
    Graphics::ScopedSaveState ss (g);
    g.addTransform (AffineTransform::rotation (degreesToRadians (rot), a.getCentreX(), a.getCentreY()));
    g.setColour (ink); g.fillRoundedRectangle (a.translated (2.5f, 3.0f), 4.0f);
    g.setColour (Colours::white); g.fillRoundedRectangle (a.expanded (2.5f), 5.0f);
    g.setColour (fill); g.fillRoundedRectangle (a, 3.5f);
    g.setColour (textCol); g.setFont (display (a.getHeight() * 0.62f));
    g.drawFittedText (text, a.toNearestInt(), Justification::centred, 1, 0.6f);
}

void drawSpeaker (Graphics& g, Rectangle<float> a, float pulse)
{
    auto c = a.getCentre();
    const float r = jmin (a.getWidth(), a.getHeight()) * 0.5f;
    g.setColour (ink); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setGradientFill (ColourGradient (Colour (0xff3a3446), c.x - r * 0.4f, c.y - r * 0.4f, Colour (0xff12101a), c.x + r, c.y + r, true));
    g.fillEllipse (c.x - r * 0.93f, c.y - r * 0.93f, r * 1.86f, r * 1.86f);
    const float cone = r * (0.62f + 0.04f * pulse);
    g.setGradientFill (ColourGradient (Colour (0xff6c6480), c.x - cone * 0.3f, c.y - cone * 0.3f, Colour (0xff1b1726), c.x + cone, c.y + cone, true));
    g.fillEllipse (c.x - cone, c.y - cone, cone * 2, cone * 2);
    g.setColour (ink); g.drawEllipse (c.x - cone, c.y - cone, cone * 2, cone * 2, 2.0f);
    g.setColour (pink); g.fillEllipse (c.x - r * 0.2f, c.y - r * 0.2f, r * 0.4f, r * 0.4f);
    g.setColour (Colours::white.withAlpha (0.4f)); g.fillEllipse (c.x - r * 0.12f, c.y - r * 0.15f, r * 0.12f, r * 0.12f);
    g.setColour (Colours::white.withAlpha (0.08f));
    for (int i = 1; i < 4; ++i) g.drawEllipse (c.x - cone * (0.45f + 0.18f * i), c.y - cone * (0.45f + 0.18f * i), cone * (0.9f + 0.36f * i), cone * (0.9f + 0.36f * i), 1.0f);
}

void drawNeonFrame (Graphics& g, Rectangle<float> a, Colour c, float corner, float th)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (c.withAlpha (0.08f * (float) (4 - i)));
        g.drawRoundedRectangle (a, corner, th + (float) i * 2.5f);
    }
    g.setColour (c.brighter (0.4f)); g.drawRoundedRectangle (a, corner, th);
}

void drawDeck (Graphics& g, Rectangle<float> a, const String& title, Colour accent)
{
    g.setColour (ink); g.fillRoundedRectangle (a.translated (4.0f, 5.0f), 10.0f);
    g.setGradientFill (ColourGradient::vertical (Colour (0xff261240), a.getY(), Colour (0xff140a24), a.getBottom()));
    g.fillRoundedRectangle (a, 10.0f);
    drawNeonFrame (g, a, accent, 10.0f, 2.0f);
    if (title.isNotEmpty())
    {
        auto tag = Rectangle<float> (a.getX() + 12.0f, a.getY() - 11.0f, 14.0f + (float) title.length() * 10.5f, 22.0f);
        g.setColour (ink); g.fillRoundedRectangle (tag.expanded (2.0f), 5.0f);
        g.setColour (accent); g.fillRoundedRectangle (tag, 4.0f);
        g.setColour (ink); g.setFont (display (17.0f));
        g.drawText (title, tag, Justification::centred);
    }
}

void drawTapeCounter (Graphics& g, Rectangle<float> a, float p, const String& text)
{
    g.setColour (ink); g.fillRoundedRectangle (a, a.getHeight() * 0.3f);
    auto inner = a.reduced (3.0f);
    g.setColour (Colour (0xff0b2a1e)); g.fillRoundedRectangle (inner, inner.getHeight() * 0.3f);
    auto bar = inner.reduced (2.0f); bar.setWidth (jmax (0.0f, bar.getWidth() * p));
    g.setGradientFill (ColourGradient::horizontal (lime.darker (0.2f), inner.getX(), yellow, inner.getRight()));
    g.fillRoundedRectangle (bar, inner.getHeight() * 0.25f);
    g.setColour (Colours::white.withAlpha (0.08f));
    for (float x = inner.getX() + 8.0f; x < inner.getRight(); x += 10.0f) g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());
    g.setColour (cream); g.setFont (display (a.getHeight() * 0.62f));
    g.drawText (text, a, Justification::centred);
}

void drawCheckerStrip (Graphics& g, Rectangle<float> a, Colour c1, Colour c2, float s)
{
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (a.toNearestInt());
    int row = 0;
    for (float y = a.getY(); y < a.getBottom(); y += s, ++row)
    {
        int col = 0;
        for (float x = a.getX(); x < a.getRight(); x += s, ++col)
        {
            g.setColour (((row + col) & 1) ? c1 : c2);
            g.fillRect (x, y, s, s);
        }
    }
}
}
