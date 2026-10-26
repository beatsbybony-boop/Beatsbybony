#include "ModelManager.h"
#include <juce_cryptography/juce_cryptography.h>
#include <array>

namespace bony
{
static const std::array<ModelInfo, ModelManager::NumModels> kModels {{
    { "vocals", "Vocals (voice) finder",
      "https://github.com/TRvlvr/model_repo/releases/download/all_public_uvr_models/UVR-MDX-NET-Voc_FT.onnx",
      66762490, 66762490, "534b2070fcc7df514b13ef660dc8cbb328679c2374d04354a5c42bb14ecce111", { "UVR-MDX-NET-Voc_FT.onnx", 7680, 3072, 256, 1.0f } },
    { "demucs", "Drums, bass and music finder (Demucs)",
      "https://huggingface.co/StemSplitio/htdemucs-onnx/resolve/main/htdemucs_fp16weights.onnx",
      0, 166000000, "", { "htdemucs_fp16weights.onnx", 0, 0, 0, 1.0f } },
    { "lead", "Lead / backing vocal splitter",
      "https://github.com/TRvlvr/model_repo/releases/download/all_public_uvr_models/UVR_MDXNET_KARA_2.onnx",
      52786726, 52786726, "bf32e15105a09c0f7dddd2b67346146334d6f3ecb399ed7638eba2ab07cbf5f4", { "UVR_MDXNET_KARA_2.onnx", 5120, 2048, 256, 1.0f } }
}};

const ModelInfo& ModelManager::info (Which w) { return kModels[(size_t) w]; }

juce::File ModelManager::modelsFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("BeatsByBony").getChildFile ("models");
}

bool ModelManager::isPresent (Which w) const
{
    const auto f = fileFor (w);
    const auto& mi = info (w);
    if (! f.existsAsFile()) return false;
    return mi.bytes > 0 ? f.getSize() == mi.bytes : f.getSize() > mi.approxBytes * 9 / 10;
}

bool ModelManager::allPresent() const
{
    for (int i = 0; i < NumModels; ++i) if (! isPresent ((Which) i)) return false;
    return true;
}

juce::int64 ModelManager::totalBytesMissing() const
{
    juce::int64 b = 0;
    for (int i = 0; i < NumModels; ++i) if (! isPresent ((Which) i)) b += info ((Which) i).approxBytes;
    return b;
}

void ModelManager::startDownload()
{
    if (! isThreadRunning()) startThread();
}

void ModelManager::run()
{
    const auto dir = modelsFolder();
    if (! dir.createDirectory().wasOk()) { setStatus ("Download failed", "Could not create " + dir.getFullPathName()); return; }

    const juce::int64 total = totalBytesMissing();
    juce::int64 doneBytes = 0;
    dlProgress = 0.0f;

    for (int i = 0; i < NumModels; ++i)
    {
        const auto w = (Which) i;
        if (isPresent (w)) continue;
        const auto& mi = info (w);
        setStatus (juce::String ("Downloading ") + mi.what + "...");

        auto stream = juce::URL (mi.url).createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs (20000).withNumRedirectsToFollow (10));
        if (stream == nullptr) { setStatus ("Download failed", "Could not connect. Check your internet, or download the files by hand (see the guide)."); return; }

        const auto part = dir.getChildFile (juce::String (mi.spec.fileName) + ".part");
        part.deleteFile();
        {
            juce::FileOutputStream out (part);
            if (! out.openedOk()) { setStatus ("Download failed", "Could not write to " + part.getFullPathName()); return; }
            char buf[1 << 16];
            juce::int64 got = 0;
            while (! stream->isExhausted())
            {
                if (threadShouldExit()) { out.flush(); part.deleteFile(); setStatus ("Download cancelled"); return; }
                const int n = stream->read (buf, (int) sizeof (buf));
                if (n < 0) break;
                if (n == 0) { wait (20); continue; }
                out.write (buf, (size_t) n);
                got += n;
                dlProgress = total > 0 ? juce::jlimit (0.0f, 1.0f, (float) ((double) (doneBytes + got) / (double) total)) : 0.0f;
                if (onChange && (got % (1 << 20)) < (1 << 16)) onChange();
            }
        }
        const bool bad = mi.bytes > 0 ? (part.getSize() != mi.bytes || juce::SHA256 (part).toHexString() != juce::String (mi.sha256))
                         : part.getSize() < mi.approxBytes * 9 / 10;
        if (bad)
        {
            part.deleteFile();
            setStatus ("Download failed", juce::String ("The file for the ") + mi.what + " came down damaged. Please try again.");
            return;
        }
        fileFor (w).deleteFile();
        part.moveFileTo (fileFor (w));
        doneBytes += mi.approxBytes;
    }
    dlProgress = 1.0f;
    setStatus ("Models ready");
}
}
