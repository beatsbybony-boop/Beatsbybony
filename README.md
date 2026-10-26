# BeatsByBony

Two separate tools, both with the 80s hip hop look (cassettes, crowns, graffiti):

| Tool | What it is | What it does |
|---|---|---|
| **Note Grabber** | VST3 plugin for FL Studio | Turns audio into MIDI. Drop in a sample (or record what you're playing), slide **quickGRAB** to choose how many notes to keep, click notes on/off, preview them, then **drag the MIDI straight into FL Studio's piano roll**. |
| **Stem Splitter** | Normal Windows program (`BeatsByBony Stem Splitter.exe`) | Splits a song into **Vocals, Lead Vocals, Backing Vocals, Drums, Bass and Other** "cassettes". Drag any cassette into FL Studio (or any folder) as a WAV, or save them all. |

You do **not** need to learn JUCE or C++. GitHub builds both for you.

---

## Getting them (no programming)

1. Open the repository's **Actions** tab and click the latest green run.
2. Scroll to **Artifacts**. Two downloads:
   - **BeatsByBony-NoteGrabber-VST3** - the plugin. Unzip it, right-click **Install-NoteGrabber.bat** > **Run as administrator**. In FL Studio: **Options > Manage plugins > Find installed plugins**, then add **BeatsByBony** to a mixer insert slot (it's an effect).
   - **BeatsByBony-StemSplitter-Windows** - the program. Unzip it anywhere (keep the two files together: the `.exe` and `onnxruntime.dll`) and double-click **BeatsByBony Stem Splitter.exe**. You can also drop a song file onto the `.exe`.
3. If Windows SmartScreen says "Windows protected your PC" for the .exe, click **More info > Run anyway** (the program isn't code-signed).

If a build shows a red cross, open the failed step, copy the last 30 lines of red text, and send them to Claude.
If Windows says something is missing when starting either tool, install the free *Microsoft Visual C++ Redistributable 2022 (x64)* from Microsoft.

---

## Note Grabber

- **LOAD** a sample, **drop** a file on the plugin window, or press **REC** to record whatever is going through the plugin's mixer track. **SYNC REC** records automatically while FL's transport plays.
- **quickGRAB** slider: up = more notes (including quiet or doubtful ones), down = only the clearest.
- **LOW / HIGH**: only keep notes in that range. **GAIN**: helps with very quiet recordings.
- Click a note to switch it on or off. Click the keys on the left to hear a pitch. **FIT** re-centres the view; mouse wheel zooms (on the waveform or the notes), Shift+wheel scrolls.
- **PLAY** auditions the grabbed notes with a built-in sound (**SOUND** picker), optionally together with the original (**HEAR SOURCE**).
- **SNAP** tidies timing to a grid when the MIDI is created. Tempo comes from FL Studio.
- Drag the **DRAG MIDI** cassette into FL's piano roll or playlist, or press **SAVE .MID**.
- Works best on one instrument at a time (melody, bass, chords). Full mixed songs give rough results; run them through the Stem Splitter first.

## Stem Splitter

1. The first time, press **DOWNLOAD 290 MB**. The separation models download once into `%APPDATA%\BeatsByBony\models`. (**MODELS FOLDER** opens it.)
2. Press **LOAD SONG** (or drop a song on the window). WAV, MP3, FLAC, OGG and AIFF work.
3. Press **SPLIT IT!**. A full song takes several minutes depending on your computer; **QUICK TEST 30s** only does the first 30 seconds so you can try it fast.
4. Tick **LISTEN** on cassettes to audition them together with **PLAY**, drag a cassette into FL as a WAV, or **SAVE ALL**.
- Drums and bass come from Demucs (a modern separation model); vocals and the lead / backing split come from the Ultimate Vocal Remover models. Stacked harmonies close to the lead are the hard case.
- If it ever stops or crashes, open `%APPDATA%\BeatsByBony\splitter-log.txt` - the last lines say how far it got. Send them to Claude.

---

## Honest notes
- Everything was built and checked on Linux (note detection, the stem pipeline with a stand-in model, and the screens were run and screenshotted there). The real Demucs model could not be downloaded in that environment, so your first split is the real test.
- Audio-to-MIDI is a quick-and-clever detector, not magic. Expect to click away a few wrong notes.
- Licences: the framework (JUCE) is free for personal and small-revenue use but needs a paid licence for bigger commercial sales; the ONNX Runtime library is MIT; Demucs is MIT; the other separation models come from the open-source Ultimate Vocal Remover project. Check these before you sell anything made with them.
