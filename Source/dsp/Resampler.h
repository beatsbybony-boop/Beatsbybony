#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace bony
{
/** Offline windowed-sinc resampler (good quality, not real-time). */
inline std::vector<float> resampleMono (const float* in, size_t numIn, double srcRate, double dstRate)
{
    if (numIn == 0) return {};
    if (std::abs (srcRate - dstRate) < 1e-6) return std::vector<float> (in, in + numIn);

    const double ratio   = dstRate / srcRate;
    const double cutoff  = std::min (1.0, ratio) * 0.96;     // relative to source Nyquist
    const int    halfLen = 24;                                 // taps each side (at the lower rate)
    const int    phases  = 512;
    const double scale   = cutoff;                             // sinc scale

    // kernel table: for fractional offset f in [0,1), tap k in [-halfLen+1 .. halfLen]
    const int taps = halfLen * 2;
    std::vector<float> table ((size_t) (phases + 1) * (size_t) taps);
    for (int p = 0; p <= phases; ++p)
    {
        const double frac = (double) p / (double) phases;
        for (int t = 0; t < taps; ++t)
        {
            const double x = (double) (t - halfLen + 1) - frac;   // distance from output position
            const double a = M_PI * x * scale;
            const double s = std::abs (a) < 1e-9 ? 1.0 : std::sin (a) / a;
            const double w = 0.42 + 0.5 * std::cos (M_PI * x / halfLen) + 0.08 * std::cos (2.0 * M_PI * x / halfLen); // Blackman
            table[(size_t) p * (size_t) taps + (size_t) t] = (float) (s * scale * (std::abs (x) < halfLen ? w : 0.0));
        }
    }

    const size_t numOut = (size_t) std::llround ((double) numIn * ratio);
    std::vector<float> out (numOut);
    for (size_t i = 0; i < numOut; ++i)
    {
        const double pos = (double) i / ratio;
        const long long i0 = (long long) std::floor (pos);
        const double frac = pos - (double) i0;
        const double pf = frac * phases;
        const int p0 = (int) pf;
        const float pw = (float) (pf - p0);
        const float* k0 = &table[(size_t) p0 * (size_t) taps];
        const float* k1 = k0 + taps;
        float acc = 0.0f;
        for (int t = 0; t < taps; ++t)
        {
            const long long idx = i0 + t - halfLen + 1;
            if (idx < 0 || idx >= (long long) numIn) continue;
            acc += in[idx] * (k0[t] + (k1[t] - k0[t]) * pw);
        }
        out[i] = acc;
    }
    return out;
}
}
