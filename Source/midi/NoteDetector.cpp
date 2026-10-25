#include "NoteDetector.h"
#include "../dsp/Resampler.h"
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace bony
{
namespace
{
constexpr double kRate = 22050.0;
constexpr int kHop = 512;
constexpr int kBands = 3;
constexpr int kMaxPoly = 8;
constexpr int kHarm = 4;
constexpr float kW[kHarm] = { 1.0f, 0.7f, 0.5f, 0.35f };

struct BandFft
{
    int order = 12, size = 4096;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, work, mags;
    float norm = 1.0f;
    double df = 0.0;

    void init (int ord)
    {
        order = ord; size = 1 << ord;
        fft = std::make_unique<juce::dsp::FFT> (ord);
        window.resize ((size_t) size);
        double sum = 0.0;
        for (int i = 0; i < size; ++i) { window[(size_t) i] = (float) (0.5 - 0.5 * std::cos (2.0 * M_PI * i / size)); sum += window[(size_t) i]; }
        norm = (float) (2.0 / sum);          // sine of amplitude A -> magnitude A
        work.assign ((size_t) size * 2, 0.0f);
        mags.assign ((size_t) size / 2 + 1, 0.0f);
        df = kRate / size;
    }

    void analyse (const std::vector<float>& x, long long centre)
    {
        std::fill (work.begin(), work.end(), 0.0f);
        const long long start = centre - size / 2;
        for (int i = 0; i < size; ++i)
        {
            const long long idx = start + i;
            if (idx >= 0 && idx < (long long) x.size()) work[(size_t) i] = x[(size_t) idx] * window[(size_t) i];
        }
        fft->performFrequencyOnlyForwardTransform (work.data(), true);
        for (size_t k = 0; k < mags.size(); ++k) mags[k] = work[k] * norm;
    }

    float peak (double f) const
    {
        const int lo = std::max (1, (int) std::floor (f * 0.982 / df));
        const int hi = std::min ((int) mags.size() - 2, (int) std::ceil (f * 1.018 / df));
        float m = 0.0f;
        for (int k = lo; k <= hi; ++k) m = std::max (m, mags[(size_t) k]);
        return m;
    }

    void subtract (double f, float amount)
    {
        const int lo = std::max (1, (int) std::floor (f * 0.982 / df));
        const int hi = std::min ((int) mags.size() - 2, (int) std::ceil (f * 1.018 / df));
        for (int k = lo; k <= hi; ++k) mags[(size_t) k] = std::max (0.0f, mags[(size_t) k] - amount);
    }
};

inline double midiToHz (int m) { return 440.0 * std::pow (2.0, (m - 69) / 12.0); }
inline int bandFor (int midi) { return midi < 48 ? 0 : (midi < 72 ? 1 : 2); }

struct Cand { int midi; float s; };
}

std::vector<GrabNote> detectNotes (const float* mono, size_t numSamples, double sampleRate,
                                   const DetectorSettings& st,
                                   const std::function<bool (float)>& progress)
{
    if (numSamples < 2048) return {};

    // ---- prepare signal: gain, band-limit, 22.05 kHz ----
    std::vector<float> src (mono, mono + numSamples);
    const float g = juce::Decibels::decibelsToGain (st.gainDb);
    for (auto& v : src) v *= g;
    std::vector<float> x = resampleMono (src.data(), src.size(), sampleRate, kRate);
    src.clear(); src.shrink_to_fit();

    std::array<BandFft, kBands> bands;
    bands[0].init (14);   // 16384 = 743 ms: low notes
    bands[1].init (13);   //  8192 = 371 ms
    bands[2].init (12);   //  4096 = 186 ms: high notes

    const int numFrames = (int) (x.size() / kHop) + 1;
    const double floorAbs = std::pow (10.0, -54.0 / 20.0);

    // ---- pass 1: per frame candidates ----
    std::vector<std::vector<Cand>> frames ((size_t) numFrames);
    float globalMax = 1.0e-9f;
    std::vector<float> sal ((size_t) (st.highestNote - st.lowestNote + 1));

    for (int f = 0; f < numFrames; ++f)
    {
        if (progress && (f % 16) == 0 && ! progress (0.9f * (float) f / (float) numFrames)) return {};

        const long long centre = (long long) f * kHop;
        for (auto& b : bands) b.analyse (x, centre);

        // quick silence check using the mid band
        float frameEnergy = 0.0f;
        for (float m : bands[1].mags) frameEnergy = std::max (frameEnergy, m);
        if (frameEnergy < (float) floorAbs) continue;

        auto salience = [&] (int midi) -> float
        {
            const double f0 = midiToHz (midi);
            auto& B = bands[(size_t) bandFor (midi)];
            float a[kHarm]; float maxA = 1.0e-12f, sum = 0.0f;
            for (int h = 0; h < kHarm; ++h)
            {
                const double fh = f0 * (h + 1);
                a[h] = fh < kRate * 0.45 ? B.peak (fh) : 0.0f;
                maxA = std::max (maxA, a[h]);
                sum += kW[h] * a[h];
            }
            const float gate = std::min (1.0f, 2.0f * a[0] / maxA);   // the fundamental must really be there
            return sum * gate;
        };

        float firstBest = 0.0f;
        for (int pick = 0; pick < kMaxPoly; ++pick)
        {
            int bestM = -1; float bestS = 0.0f;
            for (int m = st.lowestNote; m <= st.highestNote; ++m)
            {
                bool taken = false;
                for (auto& c : frames[(size_t) f]) if (c.midi == m) { taken = true; break; }
                if (taken) continue;
                const float s = salience (m);
                if (s > bestS) { bestS = s; bestM = m; }
            }
            if (bestM < 0 || bestS < (float) floorAbs) break;
            if (pick == 0) firstBest = bestS;
            else if (bestS < 0.04f * firstBest) break;

            frames[(size_t) f].push_back ({ bestM, bestS });
            globalMax = std::max (globalMax, bestS);

            // take this note's predicted harmonics out of every band so they aren't heard as notes
            const double f0 = midiToHz (bestM);
            auto& own = bands[(size_t) bandFor (bestM)];
            const float a1 = own.peak (f0);
            for (int h = 0; h < kHarm; ++h)
            {
                const float amount = a1 * kW[h] * 0.9f;
                for (auto& B : bands) B.subtract (f0 * (h + 1), amount);
            }
        }
    }

    // ---- pass 2: track candidates over time ----
    struct Track { bool on = false; int startF = 0, lastF = 0, count = 0; float peak = 0.0f, sum = 0.0f, prev = 0.0f; int gap = 0; };
    std::array<Track, 128> tr {};
    std::vector<GrabNote> notes;
    const double hopSec = (double) kHop / kRate;

    auto closeTrack = [&] (int midi, Track& t)
    {
        if (! t.on) return;
        t.on = false;
        const double halfWin = 0.25 * (double) bands[(size_t) bandFor (midi)].size / kRate;
        double start = (double) t.startF * hopSec + halfWin;
        double end   = (double) (t.lastF + 1) * hopSec - halfWin;
        if (end - start < 0.05) { const double mid = 0.5 * (start + end); start = mid - 0.04; end = mid + 0.04; if (t.count < 3) return; }
        GrabNote n;
        n.start = std::max (0.0, start);
        n.length = std::max (0.05, end - n.start);
        n.midi = midi;
        n.strength = std::min (1.0f, t.peak / globalMax);
        n.velocity = std::min (1.0f, 0.32f + 0.68f * std::pow (n.strength, 0.55f));
        notes.push_back (n);
    };

    std::array<float, 128> cur {};
    for (int f = 0; f <= numFrames; ++f)
    {
        if (progress && (f % 64) == 0 && ! progress (0.9f + 0.1f * (float) f / (float) numFrames)) return {};
        cur.fill (0.0f);
        if (f < numFrames)
            for (auto& c : frames[(size_t) f]) if (c.s >= 0.02f * globalMax) cur[(size_t) c.midi] = std::max (cur[(size_t) c.midi], c.s);

        for (int m = 0; m < 128; ++m)
        {
            auto& t = tr[(size_t) m];
            const float s = cur[(size_t) m];
            if (s > 0.0f)
            {
                if (t.on && t.prev > 0.0f && s > 1.7f * t.prev && t.prev < 0.65f * t.peak && t.count > 4)
                {   // same pitch hit again: split into two notes
                    t.lastF = f - 1; closeTrack (m, t);
                }
                if (! t.on) { t = Track(); t.on = true; t.startF = f; }
                t.lastF = f; t.gap = 0; ++t.count; t.sum += s; t.peak = std::max (t.peak, s); t.prev = s;
            }
            else if (t.on)
            {
                t.prev = 0.0f;
                if (++t.gap > 4) closeTrack (m, t);
            }
        }
    }

    // ---- clean up: drop flicker next to longer neighbouring semitones ----
    std::sort (notes.begin(), notes.end(), [] (const GrabNote& a, const GrabNote& b) { return a.start < b.start; });
    std::vector<char> kill (notes.size(), 0);
    for (size_t i = 0; i < notes.size(); ++i)
        for (size_t j = 0; j < notes.size(); ++j)
        {
            if (i == j || std::abs (notes[i].midi - notes[j].midi) != 1) continue;
            const auto& a = notes[i]; const auto& b = notes[j];
            const bool overlap = a.start < b.start + b.length && b.start < a.start + a.length;
            if (overlap && a.length < 0.12 && b.length > 1.6 * a.length && b.strength >= a.strength) kill[i] = 1;
        }
    std::vector<GrabNote> kept;
    for (size_t i = 0; i < notes.size(); ++i) if (! kill[i]) kept.push_back (notes[i]);

    // join fragments of the same pitch separated by a tiny hole (a held note that flickered)
    std::vector<GrabNote> out;
    std::array<int, 128> lastIdx; lastIdx.fill (-1);
    for (auto& n : kept)
    {
        const int li = lastIdx[(size_t) n.midi];
        if (li >= 0)
        {
            auto& p = out[(size_t) li];
            const double gap = n.start - (p.start + p.length);
            if (gap > 0.0 && gap < 0.10)
            {
                p.length = n.start + n.length - p.start;
                p.strength = std::max (p.strength, n.strength);
                p.velocity = std::max (p.velocity, n.velocity);
                continue;
            }
        }
        lastIdx[(size_t) n.midi] = (int) out.size();
        out.push_back (n);
    }
    return out;
}
}
