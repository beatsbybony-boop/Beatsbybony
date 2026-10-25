// The ONNX Runtime library is loaded late (from the plugin folder), so it must not be touched at load time.
#define ORT_API_MANUAL_INIT
#include "MdxModel.h"
#include "../dsp/Fft.h"

#include <onnxruntime_cxx_api.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <mutex>
#include <stdexcept>

namespace bony
{
struct MdxModel::Impl
{
    MdxSpec spec;
    Ort::Env& env;
    Ort::SessionOptions options;
    std::unique_ptr<Ort::Session> session;
    std::string inName, outName;
    Fft fft;
    std::vector<double> window;

    static Ort::Env& sharedEnv()
    {
        static std::once_flag once;
        std::call_once (once, [] { Ort::InitApi(); });
        static Ort::Env* env = new Ort::Env (ORT_LOGGING_LEVEL_WARNING, "BeatsByBony");   // never destroyed on purpose
        return *env;
    }

    Impl (const std::string& path, const MdxSpec& s, int threads) : spec (s), env (sharedEnv()), fft (s.nFft)
    {
        options.SetIntraOpNumThreads (std::max (1, threads));
        options.SetInterOpNumThreads (1);
        options.SetGraphOptimizationLevel (GraphOptimizationLevel::ORT_ENABLE_ALL);
        session = std::make_unique<Ort::Session> (env, std::filesystem::u8path (path).c_str(), options);

        Ort::AllocatorWithDefaultOptions alloc;
        inName  = session->GetInputNameAllocated (0, alloc).get();
        outName = session->GetOutputNameAllocated (0, alloc).get();

        window.resize ((size_t) spec.nFft);
        for (int i = 0; i < spec.nFft; ++i)   // periodic Hann, like torch.hann_window
            window[(size_t) i] = 0.5 - 0.5 * std::cos (2.0 * M_PI * (double) i / (double) spec.nFft);
    }
};

MdxModel::MdxModel (const std::string& onnxPath, const MdxSpec& spec, int numThreads)
{
    try { impl = std::make_unique<Impl> (onnxPath, spec, numThreads); }
    catch (const Ort::Exception& e) { throw std::runtime_error (std::string ("Could not load model: ") + e.what()); }
}

MdxModel::~MdxModel() = default;

size_t MdxModel::chunkCount (size_t n, const MdxSpec& s)
{
    const size_t chunk = (size_t) kMdxHop * (size_t) (s.dimT - 1);
    const size_t gen = chunk - (size_t) s.nFft;
    const size_t pad = gen - (n % gen);
    return (n + pad + gen - 1) / gen;
}

static inline double reflectSample (const std::vector<float>& c, long long idx)
{
    const long long len = (long long) c.size();
    if (idx < 0) idx = -idx;
    if (idx >= len) idx = 2 * (len - 1) - idx;
    idx = std::max<long long> (0, std::min<long long> (len - 1, idx));
    return (double) c[(size_t) idx];
}

bool MdxModel::separate (const float* inL, const float* inR, size_t n,
                         std::vector<float>& outL, std::vector<float>& outR,
                         const std::function<bool (double)>& progress)
{
    auto& I = *impl;
    const int nFft = I.spec.nFft, F = I.spec.dimF, T = I.spec.dimT, H = kMdxHop;
    const size_t chunk = (size_t) H * (size_t) (T - 1);
    const size_t trim = (size_t) nFft / 2;
    const size_t gen = chunk - 2 * trim;
    const size_t pad = gen - (n % gen);
    const size_t totalChunks = (n + pad + gen - 1) / gen;

    outL.assign (n, 0.0f);
    outR.assign (n, 0.0f);

    std::vector<float> cl (chunk), cr (chunk);
    std::vector<float> tensorIn ((size_t) 4 * (size_t) F * (size_t) T);
    std::vector<Fft::Cx> zin ((size_t) nFft), zout ((size_t) nFft);
    std::vector<double> accL (chunk + (size_t) nFft), accR (chunk + (size_t) nFft), wsum (chunk + (size_t) nFft);

    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu (OrtArenaAllocator, OrtMemTypeDefault);
    const int64_t shape[4] = { 1, 4, F, T };
    const char* inNames[]  = { I.inName.c_str() };
    const char* outNames[] = { I.outName.c_str() };

    size_t chunkIndex = 0;
    for (size_t pos = 0; pos < n + pad; pos += gen, ++chunkIndex)
    {
        if (progress && ! progress ((double) chunkIndex / (double) totalChunks)) return false;

        // chunk of [trim zeros | signal | pad zeros | trim zeros] starting at pos
        for (size_t j = 0; j < chunk; ++j)
        {
            const long long src = (long long) (pos + j) - (long long) trim;
            if (src >= 0 && (size_t) src < n) { cl[j] = inL[src]; cr[j] = inR[src]; }
            else                              { cl[j] = 0.0f;     cr[j] = 0.0f; }
        }

        // ---- STFT (center = true, reflect padding), L and R packed into one complex FFT ----
        for (int t = 0; t < T; ++t)
        {
            const long long start = (long long) t * H - nFft / 2;
            for (int j = 0; j < nFft; ++j)
            {
                const double w = I.window[(size_t) j];
                zin[(size_t) j] = Fft::Cx (reflectSample (cl, start + j) * w, reflectSample (cr, start + j) * w);
            }
            I.fft.forward (zin.data(), zout.data());
            for (int k = 0; k < F; ++k)
            {
                const Fft::Cx a = zout[(size_t) k];
                const Fft::Cx b = std::conj (zout[(size_t) ((nFft - k) % nFft)]);
                const Fft::Cx L = 0.5 * (a + b);
                const Fft::Cx R = Fft::Cx (0.0, -0.5) * (a - b);
                const size_t base = (size_t) k * (size_t) T + (size_t) t;
                tensorIn[0 * (size_t) F * (size_t) T + base] = (float) L.real();
                tensorIn[1 * (size_t) F * (size_t) T + base] = (float) L.imag();
                tensorIn[2 * (size_t) F * (size_t) T + base] = (float) R.real();
                tensorIn[3 * (size_t) F * (size_t) T + base] = (float) R.imag();
            }
        }

        // ---- model ----
        Ort::Value input = Ort::Value::CreateTensor<float> (mem, tensorIn.data(), tensorIn.size(), shape, 4);
        auto result = I.session->Run (Ort::RunOptions { nullptr }, inNames, &input, 1, outNames, 1);
        const float* spec = result[0].GetTensorData<float>();

        // ---- inverse STFT with window-envelope normalisation ----
        std::fill (accL.begin(), accL.end(), 0.0);
        std::fill (accR.begin(), accR.end(), 0.0);
        std::fill (wsum.begin(), wsum.end(), 0.0);
        for (int t = 0; t < T; ++t)
        {
            std::fill (zin.begin(), zin.end(), Fft::Cx (0.0, 0.0));
            for (int k = 0; k < F; ++k)
            {
                const size_t base = (size_t) k * (size_t) T + (size_t) t;
                Fft::Cx L (spec[0 * (size_t) F * (size_t) T + base], spec[1 * (size_t) F * (size_t) T + base]);
                Fft::Cx R (spec[2 * (size_t) F * (size_t) T + base], spec[3 * (size_t) F * (size_t) T + base]);
                if (k == 0) { L = Fft::Cx (L.real(), 0.0); R = Fft::Cx (R.real(), 0.0); }
                zin[(size_t) k] += L + Fft::Cx (0.0, 1.0) * R;
                if (k > 0)
                    zin[(size_t) (nFft - k)] += std::conj (L) + Fft::Cx (0.0, 1.0) * std::conj (R);
            }
            I.fft.inverse (zin.data(), zout.data());

            const long long start = (long long) t * H - nFft / 2;   // may be negative
            for (int j = 0; j < nFft; ++j)
            {
                const long long p = start + j + (long long) (nFft / 2);   // shifted so index >= 0
                const double w = I.window[(size_t) j];
                accL[(size_t) p] += zout[(size_t) j].real() * w;
                accR[(size_t) p] += zout[(size_t) j].imag() * w;
                wsum[(size_t) p] += w * w;
            }
        }

        const size_t shift = (size_t) (nFft / 2);
        const float comp = I.spec.compensate;
        for (size_t j = trim; j < chunk - trim; ++j)
        {
            const size_t dst = pos + (j - trim);
            if (dst >= n) break;
            const double ws = std::max (wsum[j + shift], 1e-11);
            outL[dst] = (float) (accL[j + shift] / ws) * comp;
            outR[dst] = (float) (accR[j + shift] / ws) * comp;
        }
    }

    if (progress) progress (1.0);
    return true;
}
}
