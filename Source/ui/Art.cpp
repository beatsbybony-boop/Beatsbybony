#include "Art.h"
#include "Tag.h"
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
    const int sd = label.hashCode() & 0xffff;
    Random rnd (sd + 11);
    const Colour accB = accent.withRotatedHue (0.13f), accC = accent.withRotatedHue (-0.16f);

    if (lit)
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (accent.withAlpha (0.05f * (float) (5 - i)));
            g.fillRoundedRectangle (a.expanded ((float) i * 3.0f), h * 0.1f + (float) i * 3.0f);
        }

    // paint splash behind the tape
    drawSplat (g, { a.getX() + w * 0.10f, a.getY() + h * 0.18f }, h * 0.40f, accent, sd);
    drawSplat (g, { a.getRight() - w * 0.08f, a.getY() + h * 0.30f }, h * 0.36f, accB, sd + 5);
    drawSplat (g, { a.getCentreX() + w * 0.05f, a.getBottom() - h * 0.30f }, h * 0.30f, accC, sd + 9);

    // paint runs dripping off the bottom edge
    {
        const Colour cols[] = { accent, accB, accC, accent.brighter (0.3f), accB };
        const int nd = 5;
        for (int i = 0; i < nd; ++i)
        {
            const float dx = a.getX() + w * (0.10f + 0.80f * ((float) i + 0.5f * rnd.nextFloat()) / (float) nd);
            const float len = h * (0.05f + 0.08f * rnd.nextFloat());
            const float wd = std::max (3.0f, h * (0.045f + 0.025f * rnd.nextFloat()));
            const float y0 = a.getBottom() - 4.0f;
            g.setColour (ink);
            g.fillRoundedRectangle (dx - wd * 0.5f - 1.6f, y0, wd + 3.2f, len + 3.0f, wd * 0.5f);
            g.fillEllipse (dx - wd * 0.85f - 1.6f, y0 + len - wd * 0.5f, wd * 1.7f + 3.2f, wd * 1.9f + 3.2f);
            g.setColour (cols[i % 5]);
            g.fillRoundedRectangle (dx - wd * 0.5f, y0, wd, len + 2.0f, wd * 0.5f);
            g.fillEllipse (dx - wd * 0.85f, y0 + len - wd * 0.5f, wd * 1.7f, wd * 1.9f);
            g.setColour (Colours::white.withAlpha (0.55f)); g.fillRect (dx - wd * 0.28f, y0, wd * 0.18f, len * 0.7f);
        }
    }

    // black spray overspray around the shell (soft halo + speckle)
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (ink.withAlpha (0.10f));
        g.fillRoundedRectangle (a.expanded ((float) i * 1.6f), h * 0.1f + (float) i * 1.6f);
    }
    for (int i = 0; i < 46; ++i)
    {
        const float t = rnd.nextFloat() * 4.0f;
        Point<float> p = t < 1.0f ? Point<float> (a.getX() + w * t, a.getY()) : t < 2.0f ? Point<float> (a.getRight(), a.getY() + h * (t - 1.0f))
                       : t < 3.0f ? Point<float> (a.getRight() - w * (t - 2.0f), a.getBottom()) : Point<float> (a.getX(), a.getBottom() - h * (t - 3.0f));
        const float d = 1.0f + 2.2f * rnd.nextFloat(), rr = 2.0f + 9.0f * rnd.nextFloat() * rnd.nextFloat();
        const float ang = rnd.nextFloat() * MathConstants<float>::twoPi;
        g.setColour (ink.withAlpha (0.35f + 0.5f * rnd.nextFloat()));
        g.fillEllipse (p.x + std::cos (ang) * rr, p.y + std::sin (ang) * rr, d, d);
    }

    // plastic shell, hard black outline
    g.setColour (ink); g.fillRoundedRectangle (a.translated (3.0f, 4.0f), h * 0.08f);
    g.setGradientFill (ColourGradient::vertical (Colour (0xffd9e2ee).interpolatedWith (accent, 0.12f), a.getY(), Colour (0xff98a4b8).interpolatedWith (accent, 0.12f), a.getBottom()));
    g.fillRoundedRectangle (a, h * 0.08f);
    g.setColour (ink); g.drawRoundedRectangle (a, h * 0.08f, 3.0f);
    g.setColour (Colours::white.withAlpha (0.35f)); g.drawRoundedRectangle (a.reduced (3.5f), h * 0.07f, 1.0f);

    // label band: sprayed colour with wild-style lettering
    auto lab = Rectangle<float> (a.getX() + w * 0.07f, a.getY() + h * 0.07f, w * 0.86f, h * 0.43f);
    g.setColour (ink); g.fillRoundedRectangle (lab.translated (2.0f, 2.5f), h * 0.03f);
    g.setGradientFill (ColourGradient::vertical (accent.brighter (0.55f), lab.getY(), accent.darker (0.2f), lab.getBottom()));
    g.fillRoundedRectangle (lab, h * 0.03f);
    {
        Graphics::ScopedSaveState clip (g);
        Path lp; lp.addRoundedRectangle (lab, h * 0.03f); g.reduceClipRegion (lp);
        g.setColour (Colours::white.withAlpha (0.10f));   // halftone dots
        for (float yy = lab.getY() + 3.0f; yy < lab.getBottom(); yy += 6.0f)
            for (float xx = lab.getX() + ((int) (yy / 6.0f) % 2 ? 3.0f : 0.0f); xx < lab.getRight(); xx += 6.0f)
                g.fillEllipse (xx, yy, 2.2f, 2.2f);
        g.setColour (ink.withAlpha (0.75f));              // marker lines
        g.drawLine (lab.getX() + 4.0f, lab.getBottom() - h * 0.05f, lab.getRight() - 4.0f, lab.getBottom() - h * 0.05f, 1.4f);
    }
    g.setColour (ink); g.drawRoundedRectangle (lab, h * 0.03f, 2.0f);
    {
        tag::GStyle ls; ls.fillTop = Colour (0xfffff6b0); ls.fillBottom = Colour (0xffffb000); ls.glow = Colours::white; ls.spray = false;
        ls.extrudeTop = Colour (0xff6d6b78); ls.extrudeBottom = ink; ls.thickness = 0.19f; ls.outlineW = 0.055f; ls.inlineW = 0.0f; ls.overlap = 0.0f;
        ls.depth = 0.10f; ls.drips = false; ls.glints = false; ls.spikes = false; ls.jitter = 0.4f; ls.seed = label.length() * 3;
        tag::drawGraffiti (g, label, lab.reduced (w * 0.025f, lab.getHeight() * 0.06f), ls);
    }

    // tape window with gear reels (white teeth in a black ring, like the stencils)
    auto win = Rectangle<float> (a.getX() + w * 0.18f, a.getY() + h * 0.53f, w * 0.64f, h * 0.22f);
    g.setColour (ink); g.fillRoundedRectangle (win, win.getHeight() * 0.5f);
    g.setColour (Colour (0xff3a3446)); g.drawRoundedRectangle (win.reduced (1.0f), win.getHeight() * 0.5f, 1.2f);
    const float rr = win.getHeight() * 0.42f;
    drawReel (g, { win.getX() + rr * 1.35f, win.getCentreY() }, rr, angle);
    drawReel (g, { win.getRight() - rr * 1.35f, win.getCentreY() }, rr, -angle);
    g.setColour (Colours::white.withAlpha (0.9f));   // little tape-position window in the middle
    g.fillRoundedRectangle (win.getCentreX() - rr * 0.55f, win.getCentreY() - rr * 0.45f, rr * 1.1f, rr * 0.9f, 2.0f);
    g.setColour (ink); g.fillRect (win.getCentreX() - rr * 0.55f, win.getCentreY() - rr * 0.45f, rr * 0.55f, rr * 0.9f);

    // bottom trapezoid with the four holes
    {
        Path tz;
        const float y0 = a.getY() + h * 0.79f, y1 = a.getBottom() - h * 0.04f;
        tz.startNewSubPath (a.getX() + w * 0.20f, y0); tz.lineTo (a.getRight() - w * 0.20f, y0);
        tz.lineTo (a.getRight() - w * 0.14f, y1);      tz.lineTo (a.getX() + w * 0.14f, y1); tz.closeSubPath();
        g.setColour (Colour (0xffb8c2d2).interpolatedWith (accent, 0.15f)); g.fillPath (tz);
        g.setColour (ink); g.strokePath (tz, PathStrokeType (2.0f));
        for (float f : { 0.26f, 0.38f, 0.62f, 0.74f })
            g.fillEllipse (a.getX() + w * f - h * 0.026f, (y0 + y1) * 0.5f - h * 0.026f, h * 0.052f, h * 0.052f);
    }

    // screws
    g.setColour (ink);
    for (auto p : { Point<float> (a.getX() + w * 0.04f, a.getY() + h * 0.06f), Point<float> (a.getRight() - w * 0.04f, a.getY() + h * 0.06f),
                    Point<float> (a.getX() + w * 0.04f, a.getBottom() - h * 0.06f), Point<float> (a.getRight() - w * 0.04f, a.getBottom() - h * 0.06f) })
        g.fillEllipse (p.x - 2.2f, p.y - 2.2f, 4.4f, 4.4f);

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
    // "BEATS" (pink)  "BY" (cyan)  "BONY" (gold): wild-style throw-up lettering with 3D blocks and a crown
    const float W = a.getWidth();
    auto beats = Rectangle<float> (a.getX(), a.getY(), W * 0.44f, a.getHeight());
    auto by    = Rectangle<float> (beats.getRight() - 26.0f, a.getY() + a.getHeight() * 0.22f, W * 0.15f, a.getHeight() * 0.78f);
    auto bony  = Rectangle<float> (by.getRight() - 26.0f, a.getY(), W * 0.44f, a.getHeight());

    tag::GStyle s1; s1.fillTop = Colour (0xffffa8dc); s1.fillBottom = pink; s1.glow = Colours::white;
    s1.thickness = 0.21f; s1.seed = 4; s1.depth = 0.16f; s1.overlap = 0.03f; s1.slant = 0.30f; s1.jitter = 1.0f;
    tag::GStyle s2 = s1; s2.fillTop = Colour (0xffd4fbff); s2.fillBottom = cyan; s2.seed = 8; s2.drips = false;
    tag::GStyle s3 = s1; s3.fillTop = Colour (0xfffff08a); s3.fillBottom = Colour (0xffff9f00); s3.seed = 15;

    tag::drawGraffiti (g, "BEATS", beats, s1);
    tag::drawGraffiti (g, "BY", by, s2);
    auto used = tag::drawGraffiti (g, "BONY", bony, s3);

    drawCrown (g, Rectangle<float> (used.getX() - a.getHeight() * 0.05f, used.getY() - a.getHeight() * 0.34f, a.getHeight() * 0.56f, a.getHeight() * 0.44f), true);
}

// ------------------------------------------------------------------ small decorations
void drawBackdrop (Graphics& g, int w, int h)
{
    // big faint tags sprayed on the wall behind everything
    struct T { const char* text; float x, y, wd, ht, rot; Colour a, b; int seed; };
    const T tags[] = {
        { "BEATS", 0.01f, 0.40f, 0.36f, 0.22f, -9.0f, Colour (0xffff5fae), Colour (0xffff2e93), 3 },
        { "BONY",  0.60f, 0.44f, 0.38f, 0.24f, 7.0f,  Colour (0xffffe44a), Colour (0xffff9a00), 5 },
        { "HIP HOP", 0.30f, 0.80f, 0.34f, 0.12f, -3.0f, Colour (0xff6af0ff), Colour (0xff1ee6ff), 8 },
        { "86",    0.86f, 0.74f, 0.12f, 0.16f, 12.0f, Colour (0xffb5ff5a), Colour (0xff6bd400), 2 },
        { "FRESH", 0.02f, 0.80f, 0.20f, 0.10f, 6.0f,  Colour (0xffc58bff), Colour (0xff8a3dff), 6 } };
    for (auto& t : tags)
    {
        Graphics::ScopedSaveState ss (g);
        g.beginTransparencyLayer (0.16f);
        tag::Style st; st.fillTop = t.a; st.fillBottom = t.b; st.shadow = ink; st.thickness = 0.24f; st.smallScale = 1.0f; st.drips = true; st.seed = t.seed; st.shine = false;
        auto area = Rectangle<float> (t.x * (float) w, t.y * (float) h, t.wd * (float) w, t.ht * (float) h);
        g.addTransform (AffineTransform::rotation (degreesToRadians (t.rot), area.getCentreX(), area.getCentreY()));
        tag::draw (g, t.text, area, st);
        g.endTransparencyLayer();
    }
    drawSplat (g, { (float) w * 0.93f, (float) h * 0.30f }, 34.0f, pink.withAlpha (0.35f), 11);
    drawSplat (g, { (float) w * 0.07f, (float) h * 0.62f }, 30.0f, cyan.withAlpha (0.30f), 12);
    drawSplat (g, { (float) w * 0.52f, (float) h * 0.97f }, 26.0f, yellow.withAlpha (0.30f), 13);
    drawStar (g, { (float) w * 0.97f, (float) h * 0.58f }, 16.0f, yellow.withAlpha (0.55f), ink.withAlpha (0.5f), 0.4f);
    drawStar (g, { (float) w * 0.03f, (float) h * 0.25f }, 12.0f, cyan.withAlpha (0.55f), ink.withAlpha (0.5f), 0.2f);
}

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
    g.setGradientFill (ColourGradient::vertical (Colour (0xe6261240), a.getY(), Colour (0xe6140a24), a.getBottom()));
    g.fillRoundedRectangle (a, 10.0f);
    drawNeonFrame (g, a, accent, 10.0f, 2.0f);
    if (title.isNotEmpty())
    {
        tag::GStyle ts; ts.fillTop = accent.brighter (0.6f); ts.fillBottom = accent; ts.glow = accent; ts.extrudeTop = accent.darker (0.5f); ts.extrudeBottom = ink;
        ts.seed = title.length() * 5; ts.thickness = 0.21f; ts.depth = 0.09f; ts.drips = true; ts.spray = false; ts.glints = false; ts.spikes = false; ts.overlap = 0.0f; ts.inlineW = 0.02f;
        tag::drawGraffiti (g, title, Rectangle<float> (a.getX() + 12.0f, a.getY() - 14.0f, 30.0f + (float) title.length() * 22.0f, 40.0f), ts);
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
