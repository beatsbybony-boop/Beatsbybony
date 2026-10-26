#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "midi/MidiExport.h"

namespace bony
{
BonyProcessor::BonyProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BeatsByBony", createParameterLayout())
{
    formats.registerBasicFormats();

    for (auto* id : { &ids::grabSens, &ids::grabLow, &ids::grabHigh, &ids::grabGain, &ids::snap, &ids::prevSound, &ids::prevVol, &ids::prevSource })
        apvts.addParameterListener (*id, this);

    grab.addChangeListener (this);
    startTimer (100);
}

BonyProcessor::~BonyProcessor()
{
    stopTimer();
    grab.removeChangeListener (this);
    for (auto* id : { &ids::grabSens, &ids::grabLow, &ids::grabHigh, &ids::grabGain, &ids::snap, &ids::prevSound, &ids::prevVol, &ids::prevSource })
        apvts.removeParameterListener (*id, this);
}

bool BonyProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void BonyProcessor::prepareToPlay (double sr, int block)
{
    sampleRateHz = sr;
    maxBlock = block;
    synth.prepare (sr);
    captureBuf.setSize (2, (int) (sr * 120.0), false, true, false);   // up to 2 minutes of capture
    captureLen = 0;
}

juce::AudioProcessorEditor* BonyProcessor::createEditor() { return new BonyEditor (*this); }

// ---------------------------------------------------------------- audio thread
void BonyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    ++blockCounter;

    // host transport
    bool isPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            isPlaying = pos->getIsPlaying();
            if (auto bpm = pos->getBpm()) hostBpm = *bpm;
        }
    hostPlaying = isPlaying;

    // capture (what comes in, before any processing)
    if (armed.load())
    {
        if (isPlaying && ! wasHostPlaying && ! capturing.load()) { captureLen = 0; startedBySync = true; capturing = true; captureFinished = false; }
        if (! isPlaying && wasHostPlaying && capturing.load() && startedBySync.load()) stopRequested = true;
    }
    wasHostPlaying = isPlaying;

    if (capturing.load())
    {
        int len = captureLen.load();
        const int room = captureBuf.getNumSamples() - len;
        const int take = juce::jmin (n, room);
        for (int c = 0; c < 2; ++c)
            if (take > 0) captureBuf.copyFrom (c, len, buffer, juce::jmin (c, buffer.getNumChannels() - 1), 0, take);
        captureLen = len + take;
        if (take < n || stopRequested.load()) { capturing = false; stopRequested = false; startedBySync = false; captureFinished = true; }
    }
    else if (stopRequested.load()) { stopRequested = false; captureFinished = true; }

    // level in
    {
        float pk = 0.0f;
        for (int c = 0; c < buffer.getNumChannels(); ++c) pk = juce::jmax (pk, buffer.getMagnitude (c, 0, n));
        inLevel = juce::jmax (pk, inLevel.load() * 0.93f);
    }

    renderPlayback (buffer);

    {
        float pk = 0.0f;
        for (int c = 0; c < buffer.getNumChannels(); ++c) pk = juce::jmax (pk, buffer.getMagnitude (c, 0, n));
        outLevel = juce::jmax (pk, outLevel.load() * 0.93f);
    }
}

void BonyProcessor::renderPlayback (juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    if (playLock.tryEnter()) { audioData = playData.get(); playLock.exit(); }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;

    synth.setSound ((int) apvts.getRawParameterValue (ids::prevSound)->load());
    const float vol = apvts.getRawParameterValue (ids::prevVol)->load();

    if (const int a = auditionRequest.exchange (-1); a >= 0) synth.audition (a);

    const int mode = playMode.load();
    if (mode != lastPlayMode)
    {
        lastPlayMode = mode;
        synth.allOff();
        if (mode == (int) PlayMode::Grab && audioData != nullptr) { synth.setEvents (audioData->events); synthEventsSeen = audioData->events.get(); synth.seek (playPos); }
    }
    if (audioData != nullptr && mode == (int) PlayMode::Grab && synthEventsSeen != audioData->events.get())
    {
        synth.setEvents (audioData->events); synthEventsSeen = audioData->events.get(); synth.seek (playPos);
    }

    if (seekRequested.exchange (false)) { playPos = seekTo.load(); synth.seek (playPos); }

    if (mode == (int) PlayMode::Off || audioData == nullptr)
    {
        // nothing playing: still let audition notes ring
        synth.render (L, R, n, 0.0, false, vol);
        if (mode == (int) PlayMode::Off) playPosUi = playPos;
        return;
    }

    const double sr = sampleRateHz;
    if (mode == (int) PlayMode::Grab)
    {
        const double loopLen = juce::jmax (0.5, audioData->grabLoopSeconds);
        double pos = playPos;
        int done = 0;
        while (done < n)
        {
            const int seg = juce::jmin (n - done, (int) std::ceil ((loopLen - pos) * sr));
            const int len = juce::jmax (1, seg);
            synth.render (L + done, R + done, len, pos, true, vol);
            if (audioData->source && apvts.getRawParameterValue (ids::prevSource)->load() > 0.5f)
            {
                const auto& src = *audioData->source;
                const double ratio = src.sampleRate / sr;
                const int sch = src.audio.getNumChannels();
                const int slen = src.audio.getNumSamples();
                for (int i = 0; i < len; ++i)
                {
                    const double sp = (pos + (double) i / sr) * src.sampleRate;
                    const int i0 = (int) sp;
                    if (i0 < 0 || i0 + 1 >= slen) continue;
                    const float fr = (float) (sp - i0);
                    for (int c = 0; c < 2; ++c)
                    {
                        const float* d = src.audio.getReadPointer (juce::jmin (c, sch - 1));
                        const float v = d[i0] + (d[i0 + 1] - d[i0]) * fr;
                        (c == 0 ? L : R)[done + i] += v * 0.8f;
                    }
                }
                juce::ignoreUnused (ratio);
            }
            pos += (double) len / sr; done += len;
            if (pos >= loopLen) { pos = 0.0; synth.seek (0.0); }
        }
        playPos = pos;
    }
    playPosUi = playPos;
}

// ---------------------------------------------------------------- message thread
void BonyProcessor::parameterChanged (const juce::String& id, float)
{
    if (id == ids::grabGain) { gainDirty = true; lastGainChangeMs = juce::Time::getMillisecondCounter(); }
    previewDirty = true;
}

void BonyProcessor::changeListenerCallback (juce::ChangeBroadcaster* src)
{
    if (src == &grab) previewDirty = true;
    sendChangeMessage();
}

void BonyProcessor::timerCallback()
{
    if (gainDirty.load() && juce::Time::getMillisecondCounter() - (juce::uint32) lastGainChangeMs > 350)
    {
        gainDirty = false;
        if (currentSource) grab.analyse (currentSource, apvts.getRawParameterValue (ids::grabGain)->load());
    }
    if (previewDirty.exchange (false)) rebuildPlayback();

    // capture bookkeeping
    if (captureFinished.exchange (false)) finishCapture();
    else if (stopRequested.load())
    {
        // the host may have stopped calling us: give it a moment, then finish by force
        const unsigned bc = blockCounter.load();
        if (bc == lastSeenBlockCounter) { if (++stalledTicks > 8) { stopRequested = false; capturing = false; startedBySync = false; finishCapture(); stalledTicks = 0; } }
        else stalledTicks = 0;
        lastSeenBlockCounter = bc;
    }
    else stalledTicks = 0;
}

void BonyProcessor::loadFileAsync (const juce::File& f)
{
    juce::WeakReference<BonyProcessor> self (this);
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
        juce::MessageManager::callAsync ([self, src] { if (self) self->setSource (src); });
    });
}

void BonyProcessor::setSource (SourcePtr s)
{
    setPlayMode (PlayMode::Off);
    currentSource = std::move (s);
    grab.clear();
    seekSeconds (0.0);
    if (currentSource) grab.analyse (currentSource, apvts.getRawParameterValue (ids::grabGain)->load());
    rebuildPlayback();
    sendChangeMessage();
}

void BonyProcessor::armCapture (bool shouldArm)
{
    armed = shouldArm;
    if (! shouldArm && capturing.load() && startedBySync.load()) stopRequested = true;
    sendChangeMessage();
}

void BonyProcessor::startCaptureNow()
{
    if (capturing.load()) return;
    startedBySync = false;
    captureFinished = false;
    captureLen = 0;
    capturing = true;
    sendChangeMessage();
}

void BonyProcessor::stopCaptureNow()
{
    if (capturing.load()) { stopRequested = true; lastSeenBlockCounter = blockCounter.load(); stalledTicks = 0; }
    sendChangeMessage();
}

void BonyProcessor::finishCapture()
{
    capturing = false;
    const int len = captureLen.load();
    if (len < (int) (sampleRateHz * 0.3)) { sendChangeMessage(); return; }
    auto src = std::make_shared<SourceAudio>();
    src->audio.setSize (2, len);
    for (int c = 0; c < 2; ++c) src->audio.copyFrom (c, 0, captureBuf, c, 0, len);
    src->sampleRate = sampleRateHz;
    src->name = "Capture " + juce::Time::getCurrentTime().formatted ("%H:%M:%S");
    setSource (src);
}

void BonyProcessor::setPlayMode (PlayMode m)
{
    playMode = (int) m;
    sendChangeMessage();
}

void BonyProcessor::rebuildPlayback()
{
    auto pd = std::make_shared<PlaybackData>();
    pd->source = currentSource;

    const float sens = apvts.getRawParameterValue (ids::grabSens)->load();
    const int lo = (int) apvts.getRawParameterValue (ids::grabLow)->load();
    const int hi = (int) apvts.getRawParameterValue (ids::grabHigh)->load();
    auto active = grab.getActive (sens, lo, hi);

    auto ev = std::make_shared<PreviewEvents>();
    double end = currentSource ? currentSource->seconds() : 0.0;
    for (auto& n : active)
    {
        ev->push_back ({ n.start, true, n.midi, n.velocity });
        ev->push_back ({ n.start + n.length, false, n.midi, 0.0f });
        end = juce::jmax (end, n.start + n.length);
    }
    std::sort (ev->begin(), ev->end(), [] (const PreviewEvent& a, const PreviewEvent& b)
               { return a.time < b.time || (a.time == b.time && ! a.on && b.on); });
    pd->events = ev;
    pd->grabLoopSeconds = end;

    std::shared_ptr<const PlaybackData> old;
    {
        const juce::SpinLock::ScopedLockType sl (playLock);
        old = playData;
        playData = pd;
    }
    if (old) { graveyard.push_back (old); if (graveyard.size() > 6) graveyard.erase (graveyard.begin()); }
    sendChangeMessage();
}

// ---------------------------------------------------------------- export
juce::File BonyProcessor::writeMidiToTemp()
{
    const float sens = apvts.getRawParameterValue (ids::grabSens)->load();
    const int lo = (int) apvts.getRawParameterValue (ids::grabLow)->load();
    const int hi = (int) apvts.getRawParameterValue (ids::grabHigh)->load();
    auto active = grab.getActive (sens, lo, hi);
    if (active.empty()) return {};

    static const int divs[] = { 0, 4, 8, 16, 32 };
    const int snapIdx = (int) apvts.getRawParameterValue (ids::snap)->load();

    juce::String base = "BeatsByBony";
    if (currentSource) base = juce::File::createLegalFileName (currentSource->name.upToLastOccurrenceOf (".", false, false));
    if (base.isEmpty()) base = "BeatsByBony";
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("BeatsByBony");
    dir.createDirectory();
    auto f = dir.getChildFile (base + "_grab_" + juce::String (++tempFileCounter) + ".mid");
    return writeMidiFile (active, hostBpm.load(), divs[juce::jlimit (0, 4, snapIdx)], f) ? f : juce::File();
}

// ---------------------------------------------------------------- state
void BonyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (currentSource && currentSource->file.existsAsFile()) state.setProperty ("lastFile", currentSource->file.getFullPathName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void BonyProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid())
        {
            apvts.replaceState (tree);
            const juce::File f (tree.getProperty ("lastFile").toString());
            if (f.existsAsFile())
            {
                juce::WeakReference<BonyProcessor> self (this);
                juce::MessageManager::callAsync ([self, f] { if (self && self->currentSource == nullptr) self->loadFileAsync (f); });
            }
        }
    }
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new bony::BonyProcessor(); }
