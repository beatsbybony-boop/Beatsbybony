#pragma once
#include "../Types.h"
#include "../stems/ModelManager.h"
#include "../stems/StemEngine.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>

namespace bony
{
/** Everything the stem splitter app does that is not drawing: loading a song, running the separation,
    playing the stems and saving them. */
class SplitterCore : public juce::ChangeBroadcaster,
                     private juce::ChangeListener,
                     private juce::AudioIODeviceCallback
{
public:
    SplitterCore();
    ~SplitterCore() override;

    ModelManager models;
    StemEngine stems;

    SourcePtr getSource() const { return currentSource; }
    StemSetPtr getStems() const { return currentStems; }
    void loadFileAsync (const juce::File&);
    juce::String lastError() const { const juce::ScopedLock sl (errLock); return errorText; }

    void setPlaying (bool);
    bool isPlaying() const { return playing.load(); }
    std::array<std::atomic<bool>, kNumSlots> audible;
    std::array<std::atomic<float>, kNumSlots> gain;

    bool saveStemWav (int slot, const juce::File& dest);
    juce::File writeStemToTemp (int slot);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override {}
    void audioDeviceIOCallbackWithContext (const float* const*, int, float* const* out, int numOut, int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;
    void setError (const juce::String& e) { { const juce::ScopedLock sl (errLock); errorText = e; } sendChangeMessage(); }

    juce::AudioFormatManager formats;
    juce::AudioDeviceManager devices;
    SourcePtr currentSource;
    StemSetPtr currentStems;
    mutable juce::CriticalSection errLock;
    juce::String errorText;

    juce::SpinLock stemLock;
    StemSetPtr audioStems;
    std::atomic<bool> playing { false };
    double deviceRate = 44100.0, playPos = 0.0;
    bool deviceOk = false;

    JUCE_DECLARE_WEAK_REFERENCEABLE (SplitterCore)
};
}
