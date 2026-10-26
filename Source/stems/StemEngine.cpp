#include "StemEngine.h"
#include "OrtRuntime.h"
#include "../dsp/Resampler.h"
#include <thread>

namespace bony
{
void StemEngine::start (SourcePtr src, Options o)
{
    if (isThreadRunning()) return;
    { const juce::ScopedLock sl (lock); source = std::move (src); opts = o; error.clear(); result.reset(); }
    state = State::Working;
    prog = 0.0f;
    startThread (juce::Thread::Priority::low);
}

static juce::AudioBuffer<float> toBuffer (const std::vector<float>& l, const std::vector<float>& r)
{
    juce::AudioBuffer<float> b (2, (int) l.size());
    std::copy (l.begin(), l.end(), b.getWritePointer (0));
    std::copy (r.begin(), r.end(), b.getWritePointer (1));
    return b;
}

void StemEngine::run()
{
    SourcePtr src; Options o;
    { const juce::ScopedLock sl (lock); src = source; o = opts; }
    if (src == nullptr || src->audio.getNumSamples() < 4410) { fail ("Load some audio first."); return; }
    if (! models.allPresent()) { fail ("The separation models are not downloaded yet."); return; }

    // where is onnxruntime? (next to the plugin file)
    {
        std::string err;
        const auto dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
        if (! initOnnxRuntime (dir.getFullPathName().toStdString(), err)) { fail (juce::String (err)); return; }
    }

    try
    {
        setStatus ("Getting the audio ready...");
        // ---- stereo @ 44.1 kHz ----
        const int chans = src->audio.getNumChannels();
        const int n0 = src->audio.getNumSamples();
        std::vector<float> L = resampleMono (src->audio.getReadPointer (0), (size_t) n0, src->sampleRate, kMdxSampleRate);
        std::vector<float> R = chans > 1 ? resampleMono (src->audio.getReadPointer (1), (size_t) n0, src->sampleRate, kMdxSampleRate) : L;
        if (o.quickTest) { const size_t lim = (size_t) (kMdxSampleRate * 30.0); if (L.size() > lim) { L.resize (lim); R.resize (lim); } }
        const size_t n = L.size();
        if (threadShouldExit()) { state = State::Idle; setStatus ("Cancelled"); return; }

        // ---- work plan for the progress bar (a Demucs call costs about 8 MDX calls) ----
        const double demCost = 8.0;
        double totalChunks = (double) MdxModel::chunkCount (n, ModelManager::info (ModelManager::Vocals).spec);
        if (o.instruments) totalChunks += demCost * (double) DemucsModel::chunkCount (n);
        if (o.vocalSplit)  totalChunks += (double) MdxModel::chunkCount (n, ModelManager::info (ModelManager::LeadSplit).spec);
        double doneChunks = 0.0;

        const int threads = juce::jlimit (1, 8, (int) std::thread::hardware_concurrency() / 2);
        auto runModel = [&] (ModelManager::Which w, const std::vector<float>& inL, const std::vector<float>& inR,
                             std::vector<float>& oL, std::vector<float>& oR, const char* label) -> bool
        {
            setStatus (juce::String ("Loading ") + label + "...");
            const auto& mi = ModelManager::info (w);
            MdxModel model (models.fileFor (w).getFullPathName().toStdString(), mi.spec, threads);
            setStatus (juce::String ("Splitting: ") + label);
            const double mine = (double) MdxModel::chunkCount (n, mi.spec);
            const bool ok = model.separate (inL.data(), inR.data(), n, oL, oR, [&] (double p)
            {
                prog = (float) ((doneChunks + p * mine) / totalChunks);
                return ! threadShouldExit();
            });
            doneChunks += mine;
            return ok;
        };

        std::vector<float> vL, vR, dL, dR, bL, bR, kL, kR;
        if (! runModel (ModelManager::Vocals, L, R, vL, vR, "vocals")) { state = State::Idle; setStatus ("Cancelled"); return; }

        auto out = std::make_shared<StemSet>();
        out->sampleRate = kMdxSampleRate;
        out->sourceName = src->name;
        out->stems.resize (kNumSlots);
        for (int i = 0; i < kNumSlots; ++i) out->stems[(size_t) i].name = stemSlotName (i);
        out->stems[kVocals].audio = toBuffer (vL, vR);

        if (o.instruments)
        {
            setStatus ("Loading drums, bass and music finder...");
            try
            {
                DemucsModel dem (models.fileFor (ModelManager::Demucs).getFullPathName().toStdString(), threads);
                setStatus ("Splitting: drums, bass and music");
                const double mine = demCost * (double) DemucsModel::chunkCount (n);
                std::vector<float> st[DemucsModel::NumStems][2];
                const bool ok = dem.separate (L.data(), R.data(), n, st, [&] (double p)
                {
                    prog = (float) ((doneChunks + p * mine) / totalChunks);
                    return ! threadShouldExit();
                });
                if (! ok) { state = State::Idle; setStatus ("Cancelled"); return; }
                doneChunks += mine;
                dL = st[DemucsModel::Drums][0]; dR = st[DemucsModel::Drums][1];
                bL = st[DemucsModel::Bass][0];  bR = st[DemucsModel::Bass][1];
            }
            catch (const std::exception& e)
            {
                // a damaged download: remove it so the next try fetches it again
                models.fileFor (ModelManager::Demucs).deleteFile();
                fail (juce::String ("The Demucs model could not be used (") + e.what() + "). Press DOWNLOAD again.");
                return;
            }
            // everything the vocal finder, drums and bass did not take is "other"
            std::vector<float> oL (n), oR (n);
            for (size_t i = 0; i < n; ++i) { oL[i] = L[i] - vL[i] - dL[i] - bL[i]; oR[i] = R[i] - vR[i] - dR[i] - bR[i]; }
            out->stems[kDrums].audio = toBuffer (dL, dR);
            out->stems[kBass].audio  = toBuffer (bL, bR);
            out->stems[kOther].audio = toBuffer (oL, oR);
        }
        if (o.vocalSplit)
        {
            if (! runModel (ModelManager::LeadSplit, vL, vR, kL, kR, "lead and backing vocals")) { state = State::Idle; setStatus ("Cancelled"); return; }
            // the model keeps everything except the lead voice -> that's the backing vocals
            std::vector<float> leadL (n), leadR (n);
            for (size_t i = 0; i < n; ++i) { leadL[i] = vL[i] - kL[i]; leadR[i] = vR[i] - kR[i]; }
            out->stems[kLead].audio    = toBuffer (leadL, leadR);
            out->stems[kBacking].audio = toBuffer (kL, kR);
        }

        // slots that were not made stay silent (present = false)
        for (auto& s : out->stems)
        {
            s.present = s.audio.getNumSamples() > 0;
            if (! s.present) { s.audio.setSize (2, (int) n); s.audio.clear(); }
        }

        { const juce::ScopedLock sl (lock); result = out; }
        prog = 1.0f;
        state = State::Done;
        setStatus ("Done");
    }
    catch (const std::exception& e) { fail (juce::String ("Separation failed: ") + e.what()); }
}
}
