#include "SplitterCore.h"
#include "../stems/Log.h"

namespace bony
{
SplitterCore::SplitterCore() : stems (models)
{
    formats.registerBasicFormats();
    for (auto& a : audible) a = true;
    for (auto& g : gain) g = 1.0f;
    models.onChange = [this] { sendChangeMessage(); };
    stems.addChangeListener (this);
    const auto err = devices.initialiseWithDefaultDevices (0, 2);
    deviceOk = err.isEmpty();
    logLine ("audio device: " + (deviceOk ? juce::String ("ok") : err));
    if (deviceOk) devices.addAudioCallback (this);
}

SplitterCore::~SplitterCore()
{
    models.onChange = nullptr;
    stems.removeChangeListener (this);
    stems.cancel();
    models.cancelDownload();
    devices.removeAudioCallback (this);
}

void SplitterCore::audioDeviceAboutToStart (juce::AudioIODevice* d) { deviceRate = d != nullptr ? d->getCurrentSampleRate() : 44100.0; }

void SplitterCore::audioDeviceIOCallbackWithContext (const float* const*, int, float* const* out, int numOut, int n,
                                                     const juce::AudioIODeviceCallbackContext&)
{
    for (int c = 0; c < numOut; ++c) if (out[c] != nullptr) juce::FloatVectorOperations::clear (out[c], n);
    if (! playing.load() || numOut < 1) return;

    StemSetPtr st;
    if (stemLock.tryEnter()) { st = audioStems; stemLock.exit(); }
    if (st == nullptr) return;

    const double step = st->sampleRate / deviceRate;
    const int len = st->numSamples();
    double sp = playPos * st->sampleRate;
    float* L = out[0];
    float* R = numOut > 1 ? out[1] : out[0];
    for (int i = 0; i < n; ++i)
    {
        if (sp >= len - 1) { playing = false; sp = 0.0; break; }
        const int i0 = (int) sp; const float fr = (float) (sp - i0);
        float oL = 0.0f, oR = 0.0f;
        for (int s = 0; s < (int) st->stems.size() && s < kNumSlots; ++s)
        {
            if (! audible[(size_t) s].load() || ! st->stems[(size_t) s].present) continue;
            const float g = gain[(size_t) s].load();
            const float* a = st->stems[(size_t) s].audio.getReadPointer (0);
            const float* b = st->stems[(size_t) s].audio.getReadPointer (1);
            oL += (a[i0] + (a[i0 + 1] - a[i0]) * fr) * g;
            oR += (b[i0] + (b[i0 + 1] - b[i0]) * fr) * g;
        }
        L[i] += oL; if (R != L) R[i] += oR;
        sp += step;
    }
    playPos = sp / st->sampleRate;
}

void SplitterCore::setPlaying (bool p)
{
    if (p && currentStems == nullptr) return;
    if (p && ! deviceOk) { setError ("No audio output device was found, so playback is off. You can still save the stems."); return; }
    if (! p) playing = false;
    else { playPos = 0.0; playing = true; }
    sendChangeMessage();
}

void SplitterCore::changeListenerCallback (juce::ChangeBroadcaster* src)
{
    if (src == &stems && stems.getState() == StemEngine::State::Done)
    {
        auto r = stems.getResult();
        if (r != nullptr && r != currentStems)
        {
            currentStems = r;
            { const juce::SpinLock::ScopedLockType sl (stemLock); audioStems = r; }
            logLine ("stems ready");
        }
    }
    sendChangeMessage();
}

void SplitterCore::loadFileAsync (const juce::File& f)
{
    juce::WeakReference<SplitterCore> self (this);
    logLine ("loading " + f.getFullPathName());
    juce::Thread::launch ([self, f]
    {
        if (self == nullptr) return;
        std::unique_ptr<juce::AudioFormatReader> reader (self->formats.createReaderFor (f));
        if (reader == nullptr)
        {
            juce::MessageManager::callAsync ([self] { if (self) self->setError ("That file could not be opened. Try a WAV, AIFF, FLAC, OGG or MP3."); });
            return;
        }
        const double maxSeconds = 20.0 * 60.0;
        const juce::int64 len = (juce::int64) juce::jmin ((double) reader->lengthInSamples, maxSeconds * reader->sampleRate);
        auto src = std::make_shared<SourceAudio>();
        src->audio.setSize (juce::jmin (2, (int) reader->numChannels), (int) len);
        reader->read (&src->audio, 0, (int) len, 0, true, true);
        src->sampleRate = reader->sampleRate;
        src->name = f.getFileName();
        src->file = f;
        juce::MessageManager::callAsync ([self, src]
        {
            if (self == nullptr) return;
            self->playing = false;
            self->currentSource = src;
            self->currentStems.reset();
            { const juce::SpinLock::ScopedLockType sl (self->stemLock); self->audioStems.reset(); }
            self->stems.clearResult();
            { const juce::ScopedLock sl (self->errLock); self->errorText.clear(); }
            self->sendChangeMessage();
        });
    });
}

bool SplitterCore::saveStemWav (int slot, const juce::File& dest)
{
    if (currentStems == nullptr || slot < 0 || slot >= (int) currentStems->stems.size()) return false;
    const auto& st = currentStems->stems[(size_t) slot];
    if (! st.present) return false;
    dest.deleteFile();
    std::unique_ptr<juce::FileOutputStream> os (dest.createOutputStream());
    if (os == nullptr) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), currentStems->sampleRate, 2, 24, {}, 0));
    if (w == nullptr) return false;
    os.release();
    return w->writeFromAudioSampleBuffer (st.audio, 0, st.audio.getNumSamples());
}

juce::File SplitterCore::writeStemToTemp (int slot)
{
    if (currentStems == nullptr) return {};
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("BeatsByBony");
    dir.createDirectory();
    juce::String base = juce::File::createLegalFileName (currentStems->sourceName.upToLastOccurrenceOf (".", false, false));
    if (base.isEmpty()) base = "BeatsByBony";
    auto f = dir.getChildFile (base + " - " + juce::String (stemSlotName (slot)).toLowerCase() + ".wav");
    return saveStemWav (slot, f) ? f : juce::File();
}
}
