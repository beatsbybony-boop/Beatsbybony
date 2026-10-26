#pragma once
#include "Params.h"
#include "Types.h"
#include "dsp/PreviewSynth.h"
#include "midi/GrabEngine.h"
#include <atomic>

namespace bony
{
enum class PlayMode : int { Off = 0, Grab = 1 };

class BonyProcessor : public juce::AudioProcessor,
                      public juce::ChangeBroadcaster,          // UI listens to this for "something changed"
                      private juce::Timer,
                      private juce::ChangeListener,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    BonyProcessor();
    ~BonyProcessor() override;

    // ---- AudioProcessor ----
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BeatsByBony"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- shared state used by the editor ----
    juce::AudioProcessorValueTreeState apvts;
    GrabEngine grab;

    // source audio
    SourcePtr getSource() const { return currentSource; }
    void loadFileAsync (const juce::File& f);
    void setSource (SourcePtr s);
    juce::String lastError() const { const juce::ScopedLock sl (errLock); return errorText; }
    void clearError() { const juce::ScopedLock sl (errLock); errorText.clear(); }

    // capture from the host
    void armCapture (bool shouldArm);                // start on host play, stop on host stop
    bool isArmed() const { return armed.load(); }
    void startCaptureNow();
    void stopCaptureNow();
    bool isCapturing() const { return capturing.load(); }
    double captureSeconds() const { return sampleRateHz > 0 ? captureLen.load() / sampleRateHz : 0.0; }

    // preview / playback
    void setPlayMode (PlayMode m);
    PlayMode getPlayMode() const { return (PlayMode) playMode.load(); }
    void seekSeconds (double s) { seekTo = s; seekRequested = true; }
    double getPlayPosSeconds() const { return playPosUi.load(); }
    void auditionNote (int midi) { auditionRequest = midi; }
    double getHostBpm() const { return hostBpm.load(); }
    void rebuildPlayback();                           // call after anything that changes what is heard

    // export
    juce::File writeMidiToTemp();                     // returns empty File if nothing to write

    // meters
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f };

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void parameterChanged (const juce::String&, float) override;
    void setError (const juce::String& e) { { const juce::ScopedLock sl (errLock); errorText = e; } sendChangeMessage(); }

    struct PlaybackData
    {
        SourcePtr source;
        std::shared_ptr<const PreviewEvents> events;
        double grabLoopSeconds = 0.0;
    };
    void renderPlayback (juce::AudioBuffer<float>& buffer);
    void finishCapture();

    // message-thread state
    SourcePtr currentSource;
    juce::AudioFormatManager formats;
    std::vector<std::shared_ptr<const PlaybackData>> graveyard;
    juce::int64 lastGainChangeMs = 0;
    int tempFileCounter = 0;
    mutable juce::CriticalSection errLock;
    juce::String errorText;

    // shared with the audio thread
    juce::SpinLock playLock;
    std::shared_ptr<const PlaybackData> playData;
    const PlaybackData* audioData = nullptr;
    PreviewSynth synth;
    const PreviewEvents* synthEventsSeen = nullptr;

    std::atomic<int> playMode { 0 };
    std::atomic<double> playPosUi { 0.0 };
    std::atomic<bool> seekRequested { false };
    std::atomic<double> seekTo { 0.0 };
    std::atomic<int> auditionRequest { -1 };
    double playPos = 0.0;
    int lastPlayMode = 0;

    // capture
    juce::AudioBuffer<float> captureBuf;
    std::atomic<bool> capturing { false }, armed { false }, stopRequested { false }, captureFinished { false }, startedBySync { false };
    std::atomic<int> captureLen { 0 };
    std::atomic<unsigned> blockCounter { 0 };
    unsigned lastSeenBlockCounter = 0; int stalledTicks = 0;
    bool wasHostPlaying = false;
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<bool> hostPlaying { false };

    std::atomic<bool> previewDirty { false }, gainDirty { false };
    double sampleRateHz = 44100.0;
    int maxBlock = 512;

    JUCE_DECLARE_WEAK_REFERENCEABLE (BonyProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BonyProcessor)
};
}
