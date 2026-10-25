// Developer-only checks (not part of the plugin). Run under xvfb-run on Linux.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/midi/NoteDetector.h"
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

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String mode = argc > 1 ? argv[1] : "all";
    int failures = 0;

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

    if (mode == "all" || mode == "boost")
    {
        BonyBoost b; b.prepare (44100.0, 512);
        juce::AudioBuffer<float> buf (2, 512);
        float peak = 0; bool bad = false; float inRms = 0, outRms = 0;
        for (int mode2 = 0; mode2 < 4; ++mode2)
        {
            b.reset(); peak = 0; inRms = outRms = 0;
            for (int blk = 0; blk < 200; ++blk)
            {
                for (int i = 0; i < 512; ++i)
                {
                    const float t = (float) (blk * 512 + i) / 44100.0f;
                    const float v = 0.25f * std::sin (6.28f * 110.0f * t) + 0.12f * std::sin (6.28f * 1500.0f * t) + 0.05f * std::sin (6.28f * 7000.0f * t);
                    buf.setSample (0, i, v); buf.setSample (1, i, v);
                    inRms += v * v;
                }
                b.process (buf, 80.0f, mode2, 0.0f);
                for (int i = 0; i < 512; ++i) { const float o = buf.getSample (0, i); if (! std::isfinite (o)) bad = true; peak = std::max (peak, std::abs (o)); outRms += o * o; }
            }
            std::cout << "BOOST mode " << mode2 << ": peak " << peak << " gain " << 10.0 * std::log10 (outRms / inRms) << " dB\n";
            if (bad || peak > 1.0f) ++failures;
        }
    }

    if (mode == "all" || mode == "ui" || mode == "stems")
    {
        auto proc = std::make_unique<BonyProcessor>();
        proc->prepareToPlay (44100.0, 512);

        if (mode == "stems")
        {
            juce::File wav (argv[2]);
            proc->loadFileAsync (wav);
            for (int i = 0; i < 100 && ! proc->getSource(); ++i) pump (100);
            std::cout << "STEMS: source " << (proc->getSource() ? proc->getSource()->name : "none") << " models " << proc->models.allPresent() << "\n";
            StemEngine::Options o; o.quickTest = false;
            proc->stems.start (proc->getSource(), o);
            while (proc->stems.getState() == StemEngine::State::Working) { pump (500); std::cout << "\r" << proc->stems.statusText() << " " << (int) (proc->stems.progress() * 100) << "%   " << std::flush; }
            pump (300);
            std::cout << "\nSTEMS state " << (int) proc->stems.getState() << " err '" << proc->stems.errorText() << "'\n";
            if (auto st = proc->getStems())
                for (int i = 0; i < kNumSlots; ++i)
                {
                    auto& s = st->stems[(size_t) i];
                    std::cout << "  " << s.name << " present " << s.present << " rms " << s.audio.getRMSLevel (0, 0, s.audio.getNumSamples()) << "\n";
                    proc->saveStemWav (i, juce::File (argv[3]).getChildFile (juce::String (i) + ".wav"));
                }
        }
        else
        {
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
            auto* be = dynamic_cast<BonyEditor*> (editor.get());
            const char* names[] = { "grab", "split", "boost" };
            proc->apvts.getParameter (ids::boostOn)->setValueNotifyingHost (1.0f);
            for (int p = 0; p < 3; ++p)
            {
                be->showPage (p); pump (400);
                auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
                juce::File out (juce::String (argc > 2 ? argv[2] : "/tmp") + "/ui_" + names[p] + ".png");
                out.deleteFile();
                juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
                std::cout << "UI: wrote " << out.getFullPathName() << "\n";
            }
        }
    }
    std::cout << (failures ? "FAILURES: " : "OK ") << failures << "\n";
    return failures;
}
