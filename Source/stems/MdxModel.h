#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bony
{
/** Settings for one MDX-Net style separation model (spectrogram in, spectrogram out). */
struct MdxSpec
{
    const char* fileName;   // file name inside the models folder
    int   nFft;             // STFT size the model was trained with
    int   dimF;             // frequency bins fed to the model
    int   dimT;             // time frames per model call
    float compensate;       // gain correction applied to the output stem
};

constexpr int kMdxHop = 1024;
constexpr double kMdxSampleRate = 44100.0;

class MdxModel
{
public:
    /** Loads the model. Throws std::runtime_error on failure. */
    MdxModel (const std::string& onnxPath, const MdxSpec& spec, int numThreads);
    ~MdxModel();

    /** Separates a stereo 44.1 kHz signal and returns the model's primary stem.
        progress gets 0..1 and returns false to cancel. Returns false if cancelled. */
    bool separate (const float* inL, const float* inR, size_t numSamples,
                   std::vector<float>& outL, std::vector<float>& outR,
                   const std::function<bool (double)>& progress);

    /** Number of model calls needed for numSamples. */
    static size_t chunkCount (size_t numSamples, const MdxSpec& spec);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
