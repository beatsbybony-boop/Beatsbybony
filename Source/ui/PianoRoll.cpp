#include "PianoRoll.h"
#include "Art.h"

namespace bony
{
using namespace juce;
using namespace theme;

static bool isBlack (int m) { const int k = m % 12; return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; }
static String noteName (int m) { return MidiMessage::getMidiNoteName (m, true, true, 5); }

// ------------------------------------------------------------------ waveform
WaveformView::WaveformView (BonyProcessor& p, TimeView& v) : proc (p), view (v) { startTimerHz (20); }

void WaveformView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ink); g.fillRoundedRectangle (b, 8.0f);
    g.setGradientFill (ColourGradient::vertical (Colour (0xff1a0d2e), 0, Colour (0xff0e0719), b.getHeight()));
    g.fillRoundedRectangle (b.reduced (2.0f), 7.0f);

    auto src = proc.getSource();
    if (src == nullptr)
    {
        g.setColour (dim); g.setFont (display (22.0f));
        g.drawText ("DROP A SAMPLE HERE  -  OR HIT LOAD / REC", getLocalBounds(), Justification::centred);
        return;
    }

    const auto& buf = src->audio;
    const int n = buf.getNumSamples();
    const float mid = b.getCentreY();
    const float amp = b.getHeight() * 0.42f;
    g.setColour (cyan.withAlpha (0.85f));
    for (int px = view.leftMargin; px < getWidth() - 2; ++px)
    {
        const double t0 = view.secOf ((float) px), t1 = view.secOf ((float) px + 1.0f);
        const int i0 = jlimit (0, n - 1, (int) (t0 * src->sampleRate)), i1 = jlimit (i0 + 1, n, (int) (t1 * src->sampleRate) + 1);
        if (t1 < 0 || t0 > src->seconds()) continue;
        float lo = 0, hi = 0;
        const int step = jmax (1, (i1 - i0) / 24);
        for (int c = 0; c < buf.getNumChannels(); ++c)
        {
            const float* d = buf.getReadPointer (c);
            for (int i = i0; i < i1; i += step) { lo = jmin (lo, d[i]); hi = jmax (hi, d[i]); }
        }
        g.drawVerticalLine (px, mid - hi * amp, mid - lo * amp + 1.0f);
    }
    // playhead
    const float px = view.xOf (proc.getPlayPosSeconds());
    if (px >= view.leftMargin) { g.setColour (yellow); g.drawVerticalLine ((int) px, 2.0f, b.getBottom() - 2.0f); }

    g.setColour (cream.withAlpha (0.85f)); g.setFont (body (13.0f, true));
    g.drawText (src->name + "   " + String (src->seconds(), 1) + " s", 8, 3, getWidth() - 16, 16, Justification::topLeft);
}

void WaveformView::mouseDown (const MouseEvent& e) { proc.seekSeconds (jmax (0.0, view.secOf ((float) e.x))); }
void WaveformView::mouseDrag (const MouseEvent& e) { proc.seekSeconds (jmax (0.0, view.secOf ((float) e.x))); }

// ------------------------------------------------------------------ piano roll
PianoRoll::PianoRoll (BonyProcessor& p, TimeView& v) : proc (p), view (v) { startTimerHz (30); }

void PianoRoll::currentParams (float& sens, int& lo, int& hi) const
{
    sens = proc.apvts.getRawParameterValue (ids::grabSens)->load();
    lo = (int) proc.apvts.getRawParameterValue (ids::grabLow)->load();
    hi = (int) proc.apvts.getRawParameterValue (ids::grabHigh)->load();
}

float PianoRoll::yOfNote (int midi) const
{
    const float rows = (float) (highNote - lowNote + 1);
    return (float) getHeight() - ((float) (midi - lowNote) + 1.0f) * ((float) getHeight() / rows);
}

void PianoRoll::timerCallback()
{
    const double pos = proc.getPlayPosSeconds();
    if (pos != lastPos) { lastPos = pos; repaint(); }
}

void PianoRoll::zoomToFit()
{
    auto notes = proc.grab.getAll();
    int lo = 127, hi = 0; double end = 0;
    float sens; int rl, rh; currentParams (sens, rl, rh);
    for (auto& n : notes) if (proc.grab.isActive (n, sens, rl, rh)) { lo = jmin (lo, n.midi); hi = jmax (hi, n.midi); end = jmax (end, n.start + n.length); }
    if (lo > hi) { lo = 48; hi = 72; }
    lowNote = jmax (0, lo - 3); highNote = jmin (127, hi + 3);
    if (highNote - lowNote < 24) { const int c = (lowNote + highNote) / 2; lowNote = jmax (0, c - 12); highNote = jmin (127, c + 12); }
    auto src = proc.getSource();
    const double dur = jmax (end, src ? src->seconds() : 4.0);
    view.startSec = 0.0;
    view.pixelsPerSec = jmax (10.0, (getWidth() - view.leftMargin - 8) / jmax (0.5, dur));
    repaint(); if (onViewChanged) onViewChanged();
}

int PianoRoll::noteAt (Point<float> p) const
{
    float sens; int lo, hi; currentParams (sens, lo, hi);
    auto notes = proc.grab.getAll();
    int best = -1;
    for (int i = 0; i < (int) notes.size(); ++i)
    {
        const auto& n = notes[(size_t) i];
        if (n.midi < lowNote || n.midi > highNote) continue;
        const float x0 = view.xOf (n.start), x1 = view.xOf (n.start + n.length);
        const float y0 = yOfNote (n.midi);
        const float rh = (float) getHeight() / (float) (highNote - lowNote + 1);
        if (p.x >= x0 && p.x <= jmax (x1, x0 + 5.0f) && p.y >= y0 && p.y <= y0 + rh) best = i;
    }
    return best;
}

void PianoRoll::mouseMove (const MouseEvent& e)
{
    const int h = e.x < view.leftMargin ? -1 : noteAt (e.position);
    if (h != hover) { hover = h; repaint(); }
}

void PianoRoll::mouseDown (const MouseEvent& e)
{
    if (e.x < view.leftMargin)
    {
        const float rh = (float) getHeight() / (float) (highNote - lowNote + 1);
        proc.auditionNote (highNote - (int) ((float) e.y / rh));
        return;
    }
    const int idx = noteAt (e.position);
    if (idx >= 0)
    {
        float sens; int lo, hi; currentParams (sens, lo, hi);
        proc.grab.toggle (idx, sens, lo, hi);
        const auto notes = proc.grab.getAll();
        proc.auditionNote (notes[(size_t) idx].midi);
    }
    else if (e.mods.isShiftDown() || e.getNumberOfClicks() > 1) proc.grab.resetEdits();
    else proc.seekSeconds (jmax (0.0, view.secOf ((float) e.x)));
}

void PianoRoll::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (e.mods.isShiftDown() || std::abs (w.deltaX) > std::abs (w.deltaY))
        view.startSec = jmax (0.0, view.startSec - (w.deltaX != 0 ? w.deltaX : w.deltaY) * 600.0 / view.pixelsPerSec);
    else
    {
        const double anchor = view.secOf ((float) e.x);
        view.pixelsPerSec = jlimit (4.0, 2000.0, view.pixelsPerSec * (1.0 + w.deltaY * 0.8));
        view.startSec = jmax (0.0, anchor - (e.x - view.leftMargin) / view.pixelsPerSec);
    }
    repaint(); if (onViewChanged) onViewChanged();
}

void PianoRoll::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ink); g.fillRoundedRectangle (b, 8.0f);
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (getLocalBounds().reduced (2));

    const float rows = (float) (highNote - lowNote + 1);
    const float rh = b.getHeight() / rows;
    for (int m = lowNote; m <= highNote; ++m)
    {
        const float y = yOfNote (m);
        g.setColour (isBlack (m) ? Colour (0xff120a1f) : Colour (0xff1c1030));
        g.fillRect ((float) view.leftMargin, y, b.getWidth(), rh);
        if (m % 12 == 0) { g.setColour (violet.withAlpha (0.55f)); g.drawHorizontalLine ((int) (y + rh), (float) view.leftMargin, b.getWidth()); }
    }
    // beat grid
    const double bpm = proc.getHostBpm();
    const double beat = 60.0 / jmax (30.0, bpm);
    for (double t = std::ceil (view.startSec / beat) * beat; view.xOf (t) < b.getWidth(); t += beat)
    {
        const long long k = (long long) std::llround (t / beat);
        g.setColour ((k % 4 == 0 ? pink : violet).withAlpha (k % 4 == 0 ? 0.35f : 0.18f));
        g.drawVerticalLine ((int) view.xOf (t), 0.0f, b.getHeight());
    }

    // notes
    float sens; int lo, hi; currentParams (sens, lo, hi);
    auto notes = proc.grab.getAll();
    for (int i = 0; i < (int) notes.size(); ++i)
    {
        const auto& n = notes[(size_t) i];
        if (n.midi < lowNote || n.midi > highNote) continue;
        const float x0 = view.xOf (n.start), x1 = jmax (x0 + 4.0f, view.xOf (n.start + n.length));
        if (x1 < view.leftMargin || x0 > b.getWidth()) continue;
        const float y = yOfNote (n.midi);
        auto r = Rectangle<float> (x0, y + 1.0f, x1 - x0, rh - 2.0f);
        const bool on = proc.grab.isActive (n, sens, lo, hi);
        if (on)
        {
            const auto c = pink.interpolatedWith (orange, n.velocity);
            g.setColour (c.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (2.0f), 4.0f);
            g.setGradientFill (ColourGradient::vertical (c.brighter (0.35f), r.getY(), c.darker (0.2f), r.getBottom()));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (i == hover ? Colours::white : ink); g.drawRoundedRectangle (r, 3.0f, 1.5f);
        }
        else
        {
            g.setColour (dim.withAlpha (i == hover ? 0.75f : 0.40f)); g.fillRoundedRectangle (r, 3.0f);
            g.setColour (dim.brighter (0.4f).withAlpha (0.5f)); g.drawRoundedRectangle (r, 3.0f, 1.0f);
        }
    }

    // keyboard
    g.setColour (ink); g.fillRect (0.0f, 0.0f, (float) view.leftMargin, b.getHeight());
    for (int m = lowNote; m <= highNote; ++m)
    {
        const float y = yOfNote (m);
        auto k = Rectangle<float> (1.0f, y, (float) view.leftMargin - 3.0f, rh);
        g.setColour (isBlack (m) ? Colour (0xff201a2b) : Colour (0xfff2ecde));
        g.fillRect (k.reduced (0.0f, 0.5f));
        if (m % 12 == 0) { g.setColour (ink); g.setFont (body (jmin (12.0f, rh + 2.0f), true)); g.drawText (noteName (m), k.reduced (3.0f, 0.0f), Justification::centredRight); }
    }
    g.setColour (pink); g.drawVerticalLine (view.leftMargin - 1, 0.0f, b.getHeight());

    // playhead
    const float px = view.xOf (proc.getPlayPosSeconds());
    if (px >= view.leftMargin && px <= b.getWidth()) { g.setColour (yellow); g.drawVerticalLine ((int) px, 0.0f, b.getHeight()); }

    if (proc.grab.isBusy())
    {
        g.setColour (ink.withAlpha (0.55f)); g.fillRect (b);
        g.setColour (yellow); g.setFont (display (30.0f));
        g.drawText ("GRABBING NOTES... " + String ((int) (proc.grab.progress() * 100.0f)) + "%", getLocalBounds(), Justification::centred);
    }
    else if (notes.empty())
    {
        g.setColour (dim); g.setFont (display (20.0f));
        g.drawText (proc.getSource() ? "NO NOTES FOUND - TRY MORE GAIN" : "NOTES SHOW UP HERE", getLocalBounds(), Justification::centred);
    }

}
}
