#pragma once
#include "MdxModel.h"
#include <juce_core/juce_core.h>
#include <atomic>

namespace bony
{
struct ModelInfo
{
    const char* id;
    const char* what;            // plain-English use
    const char* url;
    juce::int64 bytes;
    const char* sha256;
    MdxSpec spec;
};

/** The separation models are downloaded once (about 170 MB) into the user's AppData folder. */
class ModelManager : private juce::Thread
{
public:
    enum Which { Vocals = 0, Drums, Bass, LeadSplit, NumModels };

    ModelManager() : juce::Thread ("BeatsByBony model download") {}
    ~ModelManager() override { stopThread (4000); }

    static const ModelInfo& info (Which w);
    static juce::File modelsFolder();
    juce::File fileFor (Which w) const { return modelsFolder().getChildFile (info (w).spec.fileName); }
    bool isPresent (Which w) const;
    bool allPresent() const;
    juce::int64 totalBytesMissing() const;

    void startDownload();
    void cancelDownload() { signalThreadShouldExit(); }
    bool isDownloading() const { return isThreadRunning(); }
    float progress() const { return dlProgress.load(); }
    juce::String statusText() const { const juce::ScopedLock sl (lock); return status; }
    juce::String lastError() const { const juce::ScopedLock sl (lock); return error; }

    std::function<void()> onChange;   // called from the download thread

private:
    void run() override;
    void setStatus (const juce::String& s, const juce::String& e = {}) { { const juce::ScopedLock sl (lock); status = s; error = e; } if (onChange) onChange(); }

    std::atomic<float> dlProgress { 0.0f };
    juce::CriticalSection lock;
    juce::String status, error;
};
}
