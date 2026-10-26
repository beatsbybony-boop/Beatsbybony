// Developer-only checks (not part of the plugin). Run under xvfb-run on Linux.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/midi/NoteDetector.h"
#include "../Source/stems/MdxModel.h"
#include "../Source/stems/OrtRuntime.h"
#include "../Source/splitter/SplitterCore.h"
#include "../Source/splitter/SplitterPage.h"
#include "../Source/ui/BonyLookAndFeel.h"
using namespace bony;
#include <iostream>

using namespace bony;

static void pump (int ms) { { const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) ms; while (juce::Time::getMillisecondCounter() < end) juce::MessageManager::getInstance()->runDispatchLoopUntil (20); } }

static std::vector<float> synthMelody (double sr, std::vector<std::array<double, 3>>& truth)
{
    // {midi, start, length}
    truth = { { 60, 0.5, 0.6 }, { 64, 1.2, 0.6 }, { 67, 1.9, 0.6 }, { 72, 2.6, 0.8 },
              { 48, 0.5, 3.0 }, { 52, 0.5, 3.0 }, { 55, 0.5, 3.0 } };    // melody + a held C-major chord
    std::vector<float> x ((size_t) (sr * 4.0), 0.0f);
    for (auto& n : truth)
    {
        const double f = 440.0 * std::pow (2.0, (n[0] - 69) / 12.0);
        for (size_t i = (size_t) (n[1] * sr); i < (size_t) ((n[1] + n[2]) * sr) && i < x.size(); ++i)
        {
            const double t = (double) i / sr - n[1];
            const double env = std::min (1.0, t * 80.0) * std::exp (-t * 0.8) * std::min (1.0, (n[2] - t) * 40.0);
            double s = 0; for (int h = 1; h <= 5; ++h) s += std::sin (2 * M_PI * f * h * (double) i / sr) / h;
            x[i] += (float) (0.18 * env * s);
        }
    }
    return x;
}

// A harder, more instrument-like test: strong 2nd harmonic, detune, vibrato, plucks, repeats, noise.
static std::vector<float> synthHard (double sr, std::vector<std::array<double, 3>>& truth)
{
    truth.clear();
    juce::Random rnd (7);
    // melody: repeated notes, steps, leaps (0.35 .. 0.9 s long)
    const int mel[] = { 67, 67, 69, 71, 72, 72, 71, 67, 64, 64, 66, 69, 74, 72, 71, 67 };
    double t = 0.4;
    for (int m : mel) { const double len = 0.30 + 0.15 * (double) (rnd.nextInt (4)); truth.push_back ({ (double) m, t, len }); t += len + 0.05; }
    // bass line
    const int bass[] = { 36, 36, 43, 41, 38, 38, 43, 36 };
    double bt = 0.4;
    for (int m : bass) { truth.push_back ({ (double) m, bt, 0.8 }); bt += 0.9; }
    const double total = std::max (t, bt) + 0.5;
    std::vector<float> x ((size_t) (sr * total), 0.0f);
    int k = 0;
    for (auto& n : truth)
    {
        const bool isBass = n[0] < 50;
        const double cents = ((double) ((k * 37) % 41) - 20.0);                    // up to +-20 cents out of tune
        const double f = 440.0 * std::pow (2.0, (n[0] - 69 + cents / 100.0) / 12.0);
        const double vib = (k % 3 == 0) ? 0.012 : 0.0;
        double ph = 0.0;
        for (size_t i = (size_t) (n[1] * sr); i < (size_t) ((n[1] + n[2]) * sr) && i < x.size(); ++i)
        {
            const double tt = (double) i / sr - n[1];
            const double fi = f * (1.0 + vib * std::sin (2 * M_PI * 5.5 * tt));
            ph += 2 * M_PI * fi / sr;
            const double env = std::min (1.0, tt * 200.0) * std::exp (-tt * (isBass ? 0.9 : 1.6)) * std::min (1.0, (n[2] - tt) * 60.0);
            const double a[6] = { 0.8, 1.0, 0.5, 0.3, 0.2, 0.1 };                  // 2nd harmonic a bit louder than the fundamental
            double s = 0; for (int h = 1; h <= 6; ++h) s += a[h - 1] * std::sin (ph * h);
            x[i] += (float) ((isBass ? 0.22 : 0.16) * env * s);
        }
        ++k;
    }
    for (auto& v : x) v += (float) (0.004 * (rnd.nextFloat() * 2.0f - 1.0f));      // about -48 dB noise
    return x;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String mode = argc > 1 ? argv[1] : "all";
    int failures = 0;

    if (mode == "demucs")   // plumbing check with a stand-in model: stem k must equal gain[k] * input
    {
        { std::string e; initOnnxRuntime ("", e); }
        const size_t n = 44100 * 20;
        std::vector<float> L (n), R (n);
        for (size_t i = 0; i < n; ++i) { L[i] = 0.5f * std::sin (0.01f * (float) i); R[i] = 0.3f * std::sin (0.017f * (float) i); }
        DemucsModel m (argv[2], 2);
        std::vector<float> out[DemucsModel::NumStems][2];
        const bool ok = m.separate (L.data(), R.data(), n, out, [] (double) { return true; });
        const float gains[4] = { 0.1f, 0.2f, 0.3f, 0.4f };
        double worst = 0.0;
        for (int k = 0; k < 4; ++k) for (size_t i = 0; i < n; ++i)
            worst = std::max (worst, (double) std::abs (out[k][0][i] - gains[k] * L[i]) + std::abs (out[k][1][i] - gains[k] * R[i]));
        std::cout << "DEMUCS ok=" << ok << " chunks=" << DemucsModel::chunkCount (n) << " worst error " << worst << (worst < 1e-3 ? " PASS" : " FAIL") << "\n";
        return worst < 1e-3 ? 0 : 1;
    }
    if (mode == "all" || mode == "detect")
    {
        std::vector<std::array<double, 3>> truth;
        auto x = synthMelody (44100.0, truth);
        DetectorSettings st;
        auto notes = detectNotes (x.data(), x.size(), 44100.0, st, nullptr);
        std::cout << "DETECT: " << notes.size() << " candidates\n";
        for (auto& n : notes) if (n.strength > 0.15f) std::cout << "  midi " << n.midi << " start " << n.start << " len " << n.length << " strength " << n.strength << "\n";
        int found = 0;
        for (auto& t : truth)
            for (auto& n : notes) if (n.midi == (int) t[0] && n.strength > 0.1f && std::abs (n.start - t[1]) < 0.25) { ++found; break; }
        int wrong = 0;
        for (auto& n : notes) if (n.strength > 0.3f) { bool ok = false; for (auto& t : truth) if (n.midi == (int) t[0]) ok = true; if (! ok) ++wrong; }
        std::cout << "DETECT: found " << found << "/" << truth.size() << " true notes, " << wrong << " strong false notes\n";
        if (found < 6) ++failures;
    }

    if (mode == "all" || mode == "detect2")
    {
        std::vector<std::array<double, 3>> truth;
        auto x = synthHard (44100.0, truth);
        DetectorSettings st; st.lowestNote = 24; st.highestNote = 96;
        auto notes = detectNotes (x.data(), x.size(), 44100.0, st, nullptr);
        if (argc > 2) for (auto& tr : truth) std::cout << "  truth " << tr[0] << " " << tr[1] << " " << tr[1] + tr[2] << "\n";
        int hit = 0, matchedForTiming = 0; double sErr = 0.0, eErr = 0.0;
        std::vector<char> used (notes.size(), 0);
        for (auto& tr : truth)
        {
            int best = -1; double bd = 1e9;
            for (size_t i = 0; i < notes.size(); ++i)
                if (! used[i] && notes[i].midi == (int) tr[0] && notes[i].strength > 0.12f && std::abs (notes[i].start - tr[1]) < 0.2 && std::abs (notes[i].start - tr[1]) < bd) { bd = std::abs (notes[i].start - tr[1]); best = (int) i; }
            if (best < 0 && argc > 2) { std::cout << "  MISSED midi " << tr[0] << " start " << tr[1] << " len " << tr[2] << "\n"; for (auto& n : notes) if (n.midi == (int) tr[0] && std::abs (n.start - tr[1]) < 0.6) std::cout << "     near: start " << n.start << " len " << n.length << " s " << n.strength << "\n"; }
            if (best >= 0) { used[(size_t) best] = 1; ++hit; ++matchedForTiming; sErr += std::abs (notes[(size_t) best].start - tr[1]); eErr += std::abs (notes[(size_t) best].start + notes[(size_t) best].length - tr[1] - tr[2]); }
        }
        int fals = 0;
        for (size_t i = 0; i < notes.size(); ++i)
            if (! used[i] && notes[i].strength > 0.25f)
            {
                bool real = false;   // overlapping a true note of the same pitch counts as a split, not a false note
                for (auto& tr : truth) if (notes[i].midi == (int) tr[0] && notes[i].start < tr[1] + tr[2] && notes[i].start + notes[i].length > tr[1]) real = true;
                if (! real) { ++fals; if (argc > 2) std::cout << "  false midi " << notes[i].midi << " start " << notes[i].start << " len " << notes[i].length << " s " << notes[i].strength << "\n"; }
            }
        std::cout << "DETECT2: hit " << hit << "/" << truth.size() << "  false(strong) " << fals << "  mean start err " << (matchedForTiming ? sErr / matchedForTiming : 0.0) * 1000.0
                  << " ms  mean end err " << (matchedForTiming ? eErr / matchedForTiming : 0.0) * 1000.0 << " ms  total notes " << notes.size() << "\n";
    }

    if (mode == "all" || mode == "ui")
    {
        auto proc = std::make_unique<BonyProcessor>();
        proc->prepareToPlay (44100.0, 512);
        std::vector<std::array<double, 3>> truth;
        auto x = synthMelody (44100.0, truth);
        auto src = std::make_shared<SourceAudio>();
        src->audio.setSize (2, (int) x.size());
        for (int c = 0; c < 2; ++c) std::copy (x.begin(), x.end(), src->audio.getWritePointer (c));
        src->sampleRate = 44100.0; src->name = "demo-loop.wav";
        proc->setSource (src);
        for (int i = 0; i < 300 && (proc->grab.isBusy() || proc->grab.size() == 0); ++i) pump (100);
        std::cout << "UI: grabbed " << proc->grab.size() << " candidate notes\n";
        auto mid = proc->writeMidiToTemp();
        std::cout << "UI: midi file " << mid.getFullPathName() << " size " << mid.getSize() << "\n";

        auto editor = std::unique_ptr<juce::AudioProcessorEditor> (proc->createEditor());
        editor->setSize (BonyEditor::designW, BonyEditor::designH);
        pump (400);
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        juce::File out (juce::String (argc > 2 ? argv[2] : "/tmp") + "/ui_grab.png");
        out.deleteFile();
        juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
        std::cout << "UI: wrote " << out.getFullPathName() << "\n";
    }

    if (mode == "all" || mode == "splitui")   // the stem splitter page, drawn without needing any models
    {
        BonyLookAndFeel laf;
        juce::LookAndFeel::setDefaultLookAndFeel (&laf);
        SplitterCore core;
        SplitterPage page (core);
        page.setSize (980, 540);
        pump (300);
        auto img = page.createComponentSnapshot (page.getLocalBounds(), true, 1.0f);
        juce::File out (juce::String (argc > 2 ? argv[2] : "/tmp") + "/ui_split.png");
        out.deleteFile();
        juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
        std::cout << "UI: wrote " << out.getFullPathName() << "\n";
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }
    std::cout << (failures ? "FAILURES: " : "OK ") << failures << "\n";
    return failures;
}
