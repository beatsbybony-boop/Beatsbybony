#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>

namespace bony
{
/** One-knob "make it loud and good" processor in the spirit of a classic multiband
    compressor + limiter preset: 3-band compression, gentle saturation, brick-wall limiter.
    Modes A-D change the character; the amount knob drives how hard it works. */
class BonyBoost
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
        xoverLow.prepare (spec);
        xoverHigh.prepare (spec);
        allpassLow.prepare (spec);
        bandBuf.setSize (6, maxBlock);
        for (auto& b : bands) b.reset();
        limiterGain = 1.0f;
        smoothedAmount.reset (sampleRate, 0.05);
        smoothedAmount.setCurrentAndTargetValue (0.0f);
        lastMode = -1;
    }

    void reset()
    {
        xoverLow.reset();
        xoverHigh.reset();
        allpassLow.reset();
        for (auto& b : bands) b.reset();
        limiterGain = 1.0f;
    }

    /** amount 0..100, mode 0..3, outputDb trim. */
    void process (juce::AudioBuffer<float>& buffer, float amount, int mode, float outputDb)
    {
        const int n = buffer.getNumSamples();
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        if (numCh == 0) return;
        if (numCh == 1) buffer.setSize (2, n, true, false, true), buffer.copyFrom (1, 0, buffer, 0, 0, n);

        if (mode != lastMode) { applyMode (mode); lastMode = mode; }
        smoothedAmount.setTargetValue (juce::jlimit (0.0f, 1.0f, amount / 100.0f));

        // --- split into 3 bands (LR4): low | mid | high ---
        const double lowX = modeCfg[(size_t) mode].xLow, highX = modeCfg[(size_t) mode].xHigh;
        xoverLow.setCutoffFrequency ((float) lowX);
        xoverHigh.setCutoffFrequency ((float) highX);
        allpassLow.setCutoffFrequency ((float) highX);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* in = buffer.getReadPointer (ch);
            float* low  = bandBuf.getWritePointer (ch);
            float* mid  = bandBuf.getWritePointer (2 + ch);
            float* high = bandBuf.getWritePointer (4 + ch);
            for (int i = 0; i < n; ++i)
            {
                float lo, rest, md, hi, apA, apB;
                xoverLow.processSample (ch, in[i], lo, rest);
                xoverHigh.processSample (ch, rest, md, hi);
                allpassLow.processSample (ch, lo, apA, apB);   // keeps the low band phase-aligned
                low[i] = apA + apB; mid[i] = md; high[i] = hi;
            }
        }

        const float outGain = juce::Decibels::decibelsToGain (outputDb);
        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);
        const auto& cfg = modeCfg[(size_t) mode];
        float grAcc = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float a = smoothedAmount.getNextValue();
            float sumL = 0.0f, sumR = 0.0f;
            const float drive = juce::Decibels::decibelsToGain (a * cfg.driveDb);   // pushes the signal into the squeezers
            for (int b = 0; b < 3; ++b)
            {
                float l = bandBuf.getSample (2 * b, i) * drive, r = bandBuf.getSample (2 * b + 1, i) * drive;
                const float g = bands[(size_t) b].gain (l, r, a, cfg.band[(size_t) b], sr);
                grAcc += (1.0f - g) * (b == 0 ? 0.4f : b == 1 ? 0.4f : 0.2f);
                sumL += l * g * cfg.band[(size_t) b].tone;
                sumR += r * g * cfg.band[(size_t) b].tone;
            }

            // makeup grows with amount
            const float makeup = juce::Decibels::decibelsToGain (a * cfg.makeupDb);
            sumL *= makeup; sumR *= makeup;

            // soft saturation (blend grows with amount)
            const float satAmt = a * cfg.sat;
            if (satAmt > 0.001f)
            {
                const float drive = 1.0f + satAmt * 2.5f;
                const float norm = 1.0f / std::tanh (drive);
                sumL = juce::jmap (satAmt, sumL, std::tanh (sumL * drive) * norm);
                sumR = juce::jmap (satAmt, sumR, std::tanh (sumR * drive) * norm);
            }

            // brick-wall-ish limiter at -0.3 dBFS with instant attack, smooth release
            const float ceiling = 0.966f;
            const float peak = juce::jmax (std::abs (sumL), std::abs (sumR), 1.0e-9f);
            const float need = peak > ceiling ? ceiling / peak : 1.0f;
            if (need < limiterGain) limiterGain = need;
            else limiterGain += (1.0f - limiterGain) * limiterRelease (sr);
            L[i] = sumL * limiterGain * outGain;
            R[i] = sumR * limiterGain * outGain;
            grAcc += (1.0f - limiterGain);
        }
        grMeter.store (juce::jlimit (0.0f, 1.0f, grAcc / (float) juce::jmax (1, n) * 1.8f));
        if (buffer.getNumChannels() == 1) buffer.setSize (1, n, true);
    }

    std::atomic<float> grMeter { 0.0f };   // 0..1 for the UI

private:
    struct BandCfg { float thrAtFull, ratio, attackMs, releaseMs, tone; };
    struct ModeCfg { double xLow, xHigh; std::array<BandCfg, 3> band; float makeupDb, sat, driveDb; };

    // A = balanced, B = bass-heavy, C = bright, D = loud and crunchy
    std::array<ModeCfg, 4> modeCfg {{
        { 150.0, 2800.0, {{ { -22.0f, 3.0f, 30.0f, 160.0f, 1.00f }, { -20.0f, 2.6f, 15.0f, 110.0f, 1.00f }, { -22.0f, 2.8f,  5.0f,  70.0f, 1.00f } }}, 1.5f, 0.25f, 13.0f },
        { 190.0, 3200.0, {{ { -28.0f, 4.0f, 35.0f, 200.0f, 1.12f }, { -19.0f, 2.4f, 15.0f, 110.0f, 0.97f }, { -20.0f, 2.4f,  6.0f,  80.0f, 0.94f } }}, 2.0f, 0.30f, 13.0f },
        { 130.0, 2200.0, {{ { -20.0f, 2.6f, 30.0f, 150.0f, 0.96f }, { -20.0f, 2.8f, 12.0f, 100.0f, 1.00f }, { -26.0f, 3.6f,  4.0f,  60.0f, 1.14f } }}, 1.5f, 0.22f, 13.0f },
        { 160.0, 2600.0, {{ { -30.0f, 5.0f, 12.0f, 120.0f, 1.04f }, { -28.0f, 4.5f,  8.0f,  90.0f, 1.00f }, { -28.0f, 4.0f,  3.0f,  50.0f, 1.04f } }}, 2.0f, 0.55f, 18.0f }
    }};

    struct BandComp
    {
        float env = 0.0f, gr = 1.0f;
        void reset() { env = 0.0f; gr = 1.0f; }
        float gain (float l, float r, float amt, const BandCfg& c, double sr)
        {
            const float level = juce::jmax (std::abs (l), std::abs (r));
            const float aA = std::exp (-1.0f / (float) (sr * 0.0005));            // fast detector attack
            const float aR = std::exp (-1.0f / (float) (sr * 0.02));
            env = level > env ? aA * env + (1.0f - aA) * level : aR * env + (1.0f - aR) * level;
            const float envDb = juce::Decibels::gainToDecibels (env + 1.0e-9f);
            const float thr = juce::jmap (amt, 0.0f, c.thrAtFull);                 // 0 dB .. thrAtFull
            float over = envDb - thr;
            const float knee = 6.0f;
            float redDb = 0.0f;
            if (over > knee * 0.5f)          redDb = over * (1.0f - 1.0f / c.ratio);
            else if (over > -knee * 0.5f)    { const float x = over + knee * 0.5f; redDb = (1.0f - 1.0f / c.ratio) * x * x / (2.0f * knee); }
            const float target = juce::Decibels::decibelsToGain (-redDb);
            const float atk = std::exp (-1.0f / (float) (sr * c.attackMs * 0.001));
            const float rel = std::exp (-1.0f / (float) (sr * c.releaseMs * 0.001));
            gr = target < gr ? atk * gr + (1.0f - atk) * target : rel * gr + (1.0f - rel) * target;
            return gr;
        }
    };

    static float limiterRelease (double sr) { return 1.0f - std::exp (-1.0f / (float) (sr * 0.08)); }
    void applyMode (int) { for (auto& b : bands) b.reset(); }

    double sr = 44100.0;
    juce::dsp::LinkwitzRileyFilter<float> xoverLow, xoverHigh, allpassLow;
    juce::AudioBuffer<float> bandBuf;
    std::array<BandComp, 3> bands;
    float limiterGain = 1.0f;
    int lastMode = -1;
    juce::SmoothedValue<float> smoothedAmount;
};
}
