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

/** Demucs (htdemucs) 4-stem model: raw stereo waveform in, drums/bass/other/vocals out. */
class DemucsModel
{
public:
    enum Stem { Drums = 0, Bass, Other, Vocals, NumStems };
    DemucsModel (const std::string& onnxPath, int numThreads);   // throws std::runtime_error
    ~DemucsModel();
    /** out[stem][0=L,1=R], each numSamples long. Returns false if cancelled. */
    bool separate (const float* inL, const float* inR, size_t numSamples,
                   std::vector<float> out[NumStems][2],
                   const std::function<bool (double)>& progress);
    static size_t chunkCount (size_t numSamples);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

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
