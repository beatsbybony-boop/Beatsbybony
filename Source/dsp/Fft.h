#pragma once
#include <complex>
#include <vector>
#include <cmath>
#include <cstddef>

namespace bony
{
/** Complex FFT (double precision) for any size N = q * 2^k, with q small (1,3,5,15...).
    Used by the stem separator because the separation models need FFT sizes such as
    4096, 5120, 6144, 7680 and 8192. */
class Fft
{
public:
    using Cx = std::complex<double>;

    explicit Fft (int sizeN) : n (sizeN)
    {
        // n = q * m, m = largest power of two dividing n, q = odd remainder
        m = 1;
        q = n;
        while (q % 2 == 0) { q /= 2; m *= 2; }
        twN.resize ((size_t) n);
        for (int i = 0; i < n; ++i)
            twN[(size_t) i] = std::polar (1.0, -2.0 * M_PI * (double) i / (double) n);
        twM.resize ((size_t) std::max (1, m / 2));
        for (int i = 0; i < m / 2; ++i)
            twM[(size_t) i] = std::polar (1.0, -2.0 * M_PI * (double) i / (double) m);
        rev.resize ((size_t) m);
        int bits = 0; while ((1 << bits) < m) ++bits;
        for (int i = 0; i < m; ++i)
        {
            int r = 0;
            for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            rev[(size_t) i] = r;
        }
        sub.resize ((size_t) q * (size_t) m);
    }

    int size() const noexcept { return n; }

    /** Forward transform (e^-j). in and out may not alias. */
    void forward (const Cx* in, Cx* out)
    {
        if (q == 1) { radix2 (in, 1, out); return; }
        for (int r = 0; r < q; ++r)
            radix2 (in + r, q, sub.data() + (size_t) r * (size_t) m);
        for (int k = 0; k < n; ++k)
        {
            const int km = k % m;
            Cx acc = sub[(size_t) km];
            for (int r = 1; r < q; ++r)
                acc += twN[(size_t) (((long long) r * k) % n)] * sub[(size_t) r * (size_t) m + (size_t) km];
            out[k] = acc;
        }
    }

    /** Inverse transform, scaled by 1/N. */
    void inverse (const Cx* in, Cx* out)
    {
        tmp.resize ((size_t) n);
        for (int i = 0; i < n; ++i) tmp[(size_t) i] = std::conj (in[i]);
        forward (tmp.data(), out);
        const double s = 1.0 / (double) n;
        for (int i = 0; i < n; ++i) out[i] = std::conj (out[i]) * s;
    }

private:
    // Iterative radix-2 on a strided input, writing contiguous output of length m.
    void radix2 (const Cx* in, int stride, Cx* out)
    {
        for (int i = 0; i < m; ++i) out[rev[(size_t) i]] = in[(size_t) i * (size_t) stride];
        for (int len = 2; len <= m; len <<= 1)
        {
            const int half = len / 2, step = m / len;
            for (int i = 0; i < m; i += len)
                for (int j = 0; j < half; ++j)
                {
                    const Cx w = twM[(size_t) (j * step)];
                    const Cx u = out[i + j], v = out[i + j + half] * w;
                    out[i + j] = u + v;
                    out[i + j + half] = u - v;
                }
        }
    }

    int n = 0, m = 1, q = 1;
    std::vector<Cx> twN, twM, sub, tmp;
    std::vector<int> rev;
};
}
