#include "Tag.h"
#include <cmath>
#include <map>

namespace bony::tag
{
using namespace juce;

namespace
{
struct Stroke { bool smooth; std::vector<Point<float>> pts; };
struct Glyph { float width; std::vector<Stroke> strokes; };

using P = Point<float>;
Stroke L (std::initializer_list<P> p) { return { false, p }; }      // straight segments
Stroke S (std::initializer_list<P> p) { return { true, p }; }       // smooth curve

const std::map<char, Glyph>& glyphs()
{
    static const std::map<char, Glyph> g = []
    {
        std::map<char, Glyph> m;
        auto add = [&] (char c, float w, std::vector<Stroke> s) { m[c] = { w, std::move (s) }; };

        add ('A', 0.78f, { L ({ { 0.0f, 1.0f }, { 0.39f, 0.0f }, { 0.78f, 1.0f } }), L ({ { 0.13f, 0.68f }, { 0.65f, 0.68f } }) });
        add ('B', 0.66f, { L ({ { 0.08f, 0.0f }, { 0.08f, 1.0f } }),
                            S ({ { 0.08f, 0.0f }, { 0.42f, 0.0f }, { 0.58f, 0.13f }, { 0.58f, 0.34f }, { 0.42f, 0.5f }, { 0.08f, 0.5f } }),
                            S ({ { 0.08f, 0.5f }, { 0.46f, 0.5f }, { 0.64f, 0.66f }, { 0.64f, 0.85f }, { 0.46f, 1.0f }, { 0.08f, 1.0f } }) });
        add ('C', 0.66f, { S ({ { 0.62f, 0.14f }, { 0.42f, 0.0f }, { 0.2f, 0.06f }, { 0.07f, 0.3f }, { 0.07f, 0.7f }, { 0.2f, 0.94f }, { 0.42f, 1.0f }, { 0.62f, 0.86f } }) });
        add ('D', 0.68f, { L ({ { 0.08f, 0.0f }, { 0.08f, 1.0f } }),
                            S ({ { 0.08f, 0.0f }, { 0.38f, 0.0f }, { 0.6f, 0.2f }, { 0.64f, 0.5f }, { 0.6f, 0.8f }, { 0.38f, 1.0f }, { 0.08f, 1.0f } }) });
        add ('E', 0.6f,  { L ({ { 0.58f, 0.0f }, { 0.08f, 0.0f }, { 0.08f, 1.0f }, { 0.58f, 1.0f } }), L ({ { 0.08f, 0.5f }, { 0.48f, 0.5f } }) });
        add ('F', 0.58f, { L ({ { 0.58f, 0.0f }, { 0.08f, 0.0f }, { 0.08f, 1.0f } }), L ({ { 0.08f, 0.5f }, { 0.46f, 0.5f } }) });
        add ('G', 0.7f,  { S ({ { 0.64f, 0.14f }, { 0.44f, 0.0f }, { 0.2f, 0.06f }, { 0.07f, 0.3f }, { 0.07f, 0.7f }, { 0.2f, 0.94f }, { 0.44f, 1.0f }, { 0.64f, 0.86f }, { 0.64f, 0.56f } }), L ({ { 0.64f, 0.56f }, { 0.4f, 0.56f } }) });
        add ('H', 0.68f, { L ({ { 0.08f, 0.0f }, { 0.08f, 1.0f } }), L ({ { 0.6f, 0.0f }, { 0.6f, 1.0f } }), L ({ { 0.08f, 0.5f }, { 0.6f, 0.5f } }) });
        add ('I', 0.44f, { L ({ { 0.08f, 0.0f }, { 0.36f, 0.0f } }), L ({ { 0.22f, 0.0f }, { 0.22f, 1.0f } }), L ({ { 0.08f, 1.0f }, { 0.36f, 1.0f } }) });
        add ('J', 0.55f, { S ({ { 0.46f, 0.0f }, { 0.46f, 0.7f }, { 0.38f, 0.92f }, { 0.22f, 1.0f }, { 0.08f, 0.88f } }) });
        add ('K', 0.68f, { L ({ { 0.08f, 0.0f }, { 0.08f, 1.0f } }), L ({ { 0.62f, 0.0f }, { 0.08f, 0.58f } }), L ({ { 0.3f, 0.42f }, { 0.66f, 1.0f } }) });
        add ('L', 0.55f, { L ({ { 0.08f, 0.0f }, { 0.08f, 1.0f }, { 0.56f, 1.0f } }) });
        add ('M', 0.86f, { L ({ { 0.07f, 1.0f }, { 0.07f, 0.0f }, { 0.43f, 0.62f }, { 0.79f, 0.0f }, { 0.79f, 1.0f } }) });
        add ('N', 0.7f,  { L ({ { 0.08f, 1.0f }, { 0.08f, 0.0f }, { 0.62f, 1.0f }, { 0.62f, 0.0f } }) });
        add ('O', 0.72f, { S ({ { 0.36f, 0.0f }, { 0.58f, 0.08f }, { 0.66f, 0.5f }, { 0.58f, 0.92f }, { 0.36f, 1.0f }, { 0.14f, 0.92f }, { 0.06f, 0.5f }, { 0.14f, 0.08f }, { 0.36f, 0.0f }, { 0.58f, 0.08f } }) });
        add ('P', 0.64f, { L ({ { 0.08f, 1.0f }, { 0.08f, 0.0f } }), S ({ { 0.08f, 0.0f }, { 0.42f, 0.0f }, { 0.6f, 0.14f }, { 0.6f, 0.38f }, { 0.42f, 0.54f }, { 0.08f, 0.54f } }) });
        add ('Q', 0.74f, { S ({ { 0.36f, 0.0f }, { 0.58f, 0.08f }, { 0.66f, 0.5f }, { 0.58f, 0.92f }, { 0.36f, 1.0f }, { 0.14f, 0.92f }, { 0.06f, 0.5f }, { 0.14f, 0.08f }, { 0.36f, 0.0f }, { 0.58f, 0.08f } }), L ({ { 0.42f, 0.7f }, { 0.72f, 1.04f } }) });
        add ('R', 0.68f, { L ({ { 0.08f, 1.0f }, { 0.08f, 0.0f } }), S ({ { 0.08f, 0.0f }, { 0.42f, 0.0f }, { 0.6f, 0.14f }, { 0.6f, 0.38f }, { 0.42f, 0.54f }, { 0.08f, 0.54f } }), L ({ { 0.34f, 0.54f }, { 0.64f, 1.0f } }) });
        add ('S', 0.62f, { S ({ { 0.58f, 0.12f }, { 0.4f, 0.0f }, { 0.18f, 0.06f }, { 0.1f, 0.26f }, { 0.3f, 0.44f }, { 0.52f, 0.58f }, { 0.58f, 0.78f }, { 0.44f, 0.96f }, { 0.22f, 1.0f }, { 0.04f, 0.86f } }) });
        add ('T', 0.66f, { L ({ { 0.02f, 0.0f }, { 0.64f, 0.0f } }), L ({ { 0.33f, 0.0f }, { 0.33f, 1.0f } }) });
        add ('U', 0.68f, { S ({ { 0.08f, 0.0f }, { 0.08f, 0.66f }, { 0.2f, 0.93f }, { 0.34f, 1.0f }, { 0.48f, 0.93f }, { 0.6f, 0.66f }, { 0.6f, 0.0f } }) });
        add ('V', 0.72f, { L ({ { 0.02f, 0.0f }, { 0.36f, 1.0f }, { 0.7f, 0.0f } }) });
        add ('W', 0.98f, { L ({ { 0.02f, 0.0f }, { 0.25f, 1.0f }, { 0.49f, 0.3f }, { 0.73f, 1.0f }, { 0.96f, 0.0f } }) });
        add ('X', 0.7f,  { L ({ { 0.05f, 0.0f }, { 0.65f, 1.0f } }), L ({ { 0.65f, 0.0f }, { 0.05f, 1.0f } }) });
        add ('Y', 0.7f,  { L ({ { 0.02f, 0.0f }, { 0.35f, 0.52f }, { 0.68f, 0.0f } }), L ({ { 0.35f, 0.52f }, { 0.35f, 1.0f } }) });
        add ('Z', 0.66f, { L ({ { 0.05f, 0.0f }, { 0.6f, 0.0f }, { 0.05f, 1.0f }, { 0.62f, 1.0f } }) });

        add ('0', 0.64f, m['O'].strokes);
        m['0'].width = 0.64f;
        add ('1', 0.4f,  { L ({ { 0.06f, 0.22f }, { 0.28f, 0.0f }, { 0.28f, 1.0f } }) });
        add ('2', 0.62f, { S ({ { 0.06f, 0.2f }, { 0.2f, 0.02f }, { 0.44f, 0.02f }, { 0.58f, 0.2f }, { 0.48f, 0.46f }, { 0.06f, 1.0f } }), L ({ { 0.06f, 1.0f }, { 0.6f, 1.0f } }) });
        add ('3', 0.62f, { S ({ { 0.08f, 0.1f }, { 0.34f, 0.0f }, { 0.56f, 0.2f }, { 0.4f, 0.46f }, { 0.2f, 0.48f } }), S ({ { 0.4f, 0.46f }, { 0.6f, 0.7f }, { 0.5f, 0.94f }, { 0.28f, 1.0f }, { 0.06f, 0.9f } }) });
        add ('4', 0.66f, { L ({ { 0.48f, 1.0f }, { 0.48f, 0.0f }, { 0.04f, 0.68f }, { 0.66f, 0.68f } }) });
        add ('5', 0.62f, { L ({ { 0.58f, 0.0f }, { 0.12f, 0.0f }, { 0.09f, 0.44f } }), S ({ { 0.09f, 0.44f }, { 0.36f, 0.38f }, { 0.58f, 0.58f }, { 0.5f, 0.88f }, { 0.28f, 1.0f }, { 0.05f, 0.88f } }) });
        add ('6', 0.62f, { S ({ { 0.54f, 0.06f }, { 0.3f, 0.0f }, { 0.1f, 0.3f }, { 0.07f, 0.66f }, { 0.18f, 0.94f }, { 0.36f, 1.0f }, { 0.56f, 0.86f }, { 0.56f, 0.62f }, { 0.38f, 0.5f }, { 0.18f, 0.58f }, { 0.09f, 0.7f } }) });
        add ('7', 0.6f,  { L ({ { 0.04f, 0.0f }, { 0.58f, 0.0f }, { 0.24f, 1.0f } }) });
        add ('8', 0.62f, { S ({ { 0.31f, 0.5f }, { 0.54f, 0.36f }, { 0.5f, 0.1f }, { 0.31f, 0.0f }, { 0.12f, 0.1f }, { 0.08f, 0.36f }, { 0.31f, 0.5f }, { 0.58f, 0.72f }, { 0.52f, 0.94f }, { 0.31f, 1.0f }, { 0.1f, 0.94f }, { 0.04f, 0.72f }, { 0.31f, 0.5f }, { 0.54f, 0.36f } }) });
        add ('9', 0.62f, { S ({ { 0.08f, 0.94f }, { 0.32f, 1.0f }, { 0.52f, 0.7f }, { 0.55f, 0.34f }, { 0.44f, 0.06f }, { 0.26f, 0.0f }, { 0.06f, 0.14f }, { 0.06f, 0.38f }, { 0.24f, 0.5f }, { 0.44f, 0.42f }, { 0.53f, 0.3f } }) });

        add ('.', 0.2f,  { L ({ { 0.1f, 0.96f }, { 0.1f, 1.0f } }) });
        add ('-', 0.4f,  { L ({ { 0.06f, 0.55f }, { 0.34f, 0.55f } }) });
        add ('/', 0.45f, { L ({ { 0.42f, 0.0f }, { 0.04f, 1.0f } }) });
        add ('+', 0.55f, { L ({ { 0.05f, 0.5f }, { 0.5f, 0.5f } }), L ({ { 0.27f, 0.25f }, { 0.27f, 0.75f } }) });
        add ('>', 0.6f,  { L ({ { 0.05f, 0.15f }, { 0.52f, 0.5f }, { 0.05f, 0.85f } }) });
        add (':', 0.2f,  { L ({ { 0.1f, 0.25f }, { 0.1f, 0.29f } }), L ({ { 0.1f, 0.74f }, { 0.1f, 0.78f } }) });
        add ('%', 0.78f, { L ({ { 0.7f, 0.0f }, { 0.08f, 1.0f } }), S ({ { 0.18f, 0.02f }, { 0.28f, 0.1f }, { 0.18f, 0.2f }, { 0.08f, 0.1f }, { 0.18f, 0.02f } }), S ({ { 0.6f, 0.8f }, { 0.7f, 0.9f }, { 0.6f, 1.0f }, { 0.5f, 0.9f }, { 0.6f, 0.8f } }) });
        add ('!', 0.2f,  { L ({ { 0.1f, 0.0f }, { 0.1f, 0.68f } }), L ({ { 0.1f, 0.96f }, { 0.1f, 1.0f } }) });
        add ('\'', 0.2f, { L ({ { 0.1f, 0.0f }, { 0.08f, 0.24f } }) });
        return m;
    }();
    return g;
}

/** Catmull-Rom spline through the points as a Path. */
void addStroke (Path& path, const Stroke& s, float sx, float sy, float ox, float oy)
{
    auto at = [&] (size_t i) { const auto& p = s.pts[std::min (i, s.pts.size() - 1)]; return Point<float> (ox + p.x * sx, oy + p.y * sy); };
    path.startNewSubPath (at (0));
    if (! s.smooth || s.pts.size() < 3)
    {
        for (size_t i = 1; i < s.pts.size(); ++i) path.lineTo (at (i));
        if (s.pts.size() == 1 || (s.pts.size() == 2 && s.pts[0] == s.pts[1])) path.lineTo (at (0) + Point<float> (0.01f, 0.0f));
        return;
    }
    for (size_t i = 0; i + 1 < s.pts.size(); ++i)
    {
        const auto p0 = at (i == 0 ? 0 : i - 1), p1 = at (i), p2 = at (i + 1), p3 = at (i + 2 >= s.pts.size() ? s.pts.size() - 1 : i + 2);
        path.cubicTo (p1 + (p2 - p0) / 6.0f, p2 - (p3 - p1) / 6.0f, p2);
    }
}

struct Laid { char c; float x, scale, rot, dy; };
}

Rectangle<float> draw (Graphics& g, const String& textIn, Rectangle<float> area, const Style& st)
{
    const String text = textIn;
    Random rnd (st.seed);
    const auto& table = glyphs();

    // lay out letters on a unit line (height 1)
    std::vector<Laid> letters;
    float x = 0.0f;
    const float gap = 0.08f + st.thickness * 0.6f;
    for (auto ch : text)
    {
        if (ch == ' ') { x += 0.36f; continue; }
        const char up = (char) CharacterFunctions::toUpperCase (ch);
        auto it = table.find (up);
        if (it == table.end()) continue;
        const float sc = CharacterFunctions::isLowerCase (ch) ? st.smallScale : 1.0f;     // capitals big, lower-case small
        Laid l { up, x, sc, (rnd.nextFloat() - 0.5f) * 0.12f * st.jitter, (rnd.nextFloat() - 0.5f) * 0.06f * st.jitter };
        letters.push_back (l);
        x += it->second.width * sc + gap;
    }
    if (letters.empty()) return {};
    x -= gap;

    const float pad = st.thickness * 0.5f + st.outlineWidth + st.rimWidth + std::max (st.shadowDx, st.shadowDy);
    const float slantExtra = st.slant;
    const float unitsW = x + slantExtra + pad * 2.0f, unitsH = 1.0f + pad * 2.0f;
    const float scale = std::min (area.getWidth() / unitsW, area.getHeight() / unitsH);
    const float H = scale;                          // pixel height of a full letter
    const float ox = area.getCentreX() - (unitsW * scale) * 0.5f + pad * scale;
    const float baseY = area.getCentreY() + (unitsH * scale) * 0.5f - pad * scale;

    auto buildAll = [&] (Path& all)
    {
        for (auto& l : letters)
        {
            const auto& gl = table.at (l.c);
            Path p;
            const float lh = H * l.scale;
            for (auto& s : gl.strokes) addStroke (p, s, lh, lh, 0.0f, 0.0f);
            // bottom-align small letters, rotate about their centre, then slant
            const float gw = gl.width * lh;
            AffineTransform t = AffineTransform::translation (-gw * 0.5f, -lh)
                                    .followedBy (AffineTransform::rotation (l.rot))
                                    .followedBy (AffineTransform::translation (gw * 0.5f, lh))
                                    .followedBy (AffineTransform::translation (ox + l.x * H, baseY - lh + l.dy * H))
                                    .followedBy (AffineTransform::shear (-st.slant, 0.0f).translated (st.slant * baseY, 0.0f));
            p.applyTransform (t);
            all.addPath (p);
        }
    };
    Path all; buildAll (all);

    const float w = st.thickness * H;
    auto stroke = [] (float width) { return PathStrokeType (width, PathStrokeType::curved, PathStrokeType::rounded); };

    // shadow, outline, rim, fill
    {
        Path sh (all); sh.applyTransform (AffineTransform::translation (st.shadowDx * H, st.shadowDy * H));
        g.setColour (st.outline);  g.strokePath (sh, stroke (w + st.outlineWidth * 2.0f * H));
        g.setColour (st.shadow);   g.strokePath (sh, stroke (w + st.outlineWidth * 2.0f * H - 3.0f));
    }
    g.setColour (st.outline); g.strokePath (all, stroke (w + st.outlineWidth * 2.0f * H));
    g.setColour (st.rim);     g.strokePath (all, stroke (w + st.rimWidth * 2.0f * H));
    auto bounds = all.getBounds();
    g.setGradientFill (ColourGradient::vertical (st.fillTop, bounds.getY(), st.fillBottom, bounds.getBottom()));
    g.strokePath (all, stroke (w));

    if (st.shine)   // thin bright line through the middle of every stroke
    {
        g.setColour (Colours::white.withAlpha (0.45f));
        Path hl; AffineTransform up = AffineTransform::translation (-w * 0.16f, -w * 0.18f);
        hl.addPath (all, up);
        g.strokePath (hl, stroke (std::max (1.0f, w * 0.14f)));
    }

    if (st.drips)
    {
        Random r (st.seed * 7 + 3);
        const int n = std::max (2, (int) (letters.size() / 2));
        for (int i = 0; i < n; ++i)
        {
            auto& l = letters[(size_t) r.nextInt ((int) letters.size())];
            const float dx = ox + (l.x + 0.1f + 0.3f * r.nextFloat()) * H;
            const float len = H * (0.12f + 0.28f * r.nextFloat());
            const float wd = std::max (2.0f, w * (0.22f + 0.1f * r.nextFloat()));
            const float y0 = baseY + w * 0.15f;
            g.setColour (st.outline);
            g.fillRoundedRectangle (dx - wd * 0.5f - 1.5f, y0 - 2.0f, wd + 3.0f, len + 3.0f, wd * 0.5f);
            g.fillEllipse (dx - wd * 0.85f - 1.5f, y0 + len - wd * 0.4f, wd * 1.7f + 3.0f, wd * 1.7f + 3.0f);
            g.setColour (st.fillBottom);
            g.fillRoundedRectangle (dx - wd * 0.5f, y0 - 2.0f, wd, len + 1.0f, wd * 0.5f);
            g.fillEllipse (dx - wd * 0.85f, y0 + len - wd * 0.4f, wd * 1.7f, wd * 1.7f);
        }
    }
    return bounds;
}

void drawSimple (Graphics& g, const String& text, Rectangle<float> area, Colour fill, Colour outline, float thickness)
{
    Style s; s.fillTop = fill.brighter (0.25f); s.fillBottom = fill; s.outline = outline; s.rim = fill.brighter (0.6f).withAlpha (0.0f);
    s.thickness = thickness; s.outlineWidth = 0.055f; s.rimWidth = 0.0f; s.shadowDx = 0.0f; s.shadowDy = 0.0f; s.shadow = Colours::transparentBlack;
    s.slant = 0.12f; s.jitter = 0.4f; s.shine = false;
    draw (g, text, area, s);
}
}
