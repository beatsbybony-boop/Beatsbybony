# BeatsByBony (VST3 for FL Studio)

One plugin, three screens, 80s hip hop look:

| Screen | What it does |
|---|---|
| **GRAB** | Turns audio into MIDI. Drop in a sample (or record what you're playing), slide **quickGRAB** to choose how many notes to keep, click notes on/off, preview them, then **drag the MIDI straight into FL Studio's piano roll**. |
| **SPLIT** | Splits a song into **Vocals, Lead Vocals, Backing Vocals, Drums, Bass and Other** "cassettes". Drag any cassette into your playlist as a WAV, or save them all. |
| **BOOST** | One-knob "make it loud and punchy" (multiband squeeze + warmth + limiter) with four flavours, A to D. Works live on whatever passes through the plugin. |

You do **not** need to learn JUCE or C++. GitHub builds the plugin for you.

---

## Getting the plugin (no programming, about 10 minutes)

1. Make a free account at **github.com**.
2. Click **+** (top right) > **New repository**. Name it `BeatsByBony`, keep it **Public** (public repos build for free), click **Create repository**.
3. On the new repo page click **uploading an existing file**. Unzip this project on your computer, then drag **everything inside the `BeatsByBony` folder** (including the `.github` folder, `Source`, `installer`, `CMakeLists.txt`) into the browser window. If you can't see `.github` in your file explorer, turn on "Show hidden items". Click **Commit changes**.
4. Click the **Actions** tab. The build starts by itself (if it asks, click the green button to enable workflows). It takes roughly 10 to 15 minutes. A green tick means it worked.
5. Click the finished run, scroll to **Artifacts**, download **BeatsByBony-Windows-VST3**, and unzip it.
6. Right-click **Install-BeatsByBony.bat** > **Run as administrator**. (Or copy the `BeatsByBony.vst3` folder to `C:\Program Files\Common Files\VST3` yourself.)
7. In FL Studio: **Options > Manage plugins > Find installed plugins**. Add **BeatsByBony** to a mixer insert slot (it's an effect).

If the build shows a red cross, open the failed step, copy the last 30 lines of red text, and send them to Claude. Fixes are usually one line.

If FL says the plugin can't load, install the free *Microsoft Visual C++ Redistributable 2022 (x64)* from Microsoft and rescan.

---

## How to use it

### GRAB (audio to MIDI)
- **LOAD** a sample, **drop** a file on the plugin window, or press **REC** to record whatever is going through the plugin's mixer track. **SYNC REC** records automatically while FL's transport plays.
- **quickGRAB** slider: up = more notes (including quiet or doubtful ones), down = only the clearest.
- **LOW / HIGH**: only keep notes in that range. **GAIN**: helps with very quiet recordings.
- Click a note to switch it on or off. Click the keys on the left to hear a pitch. **FIT** re-centres the view; mouse wheel zooms (on the waveform or the notes), Shift+wheel scrolls.
- **PLAY** auditions the grabbed notes with a built-in sound (**SOUND** picker), optionally together with the original (**HEAR SOURCE**).
- **SNAP** tidies timing to a grid when the MIDI is created. Tempo comes from FL Studio.
- Drag the **DRAG MIDI** cassette into FL's piano roll or playlist, or press **SAVE .MID**.
- Works best on one instrument at a time (melody, bass, chords). Full mixed songs give rough results; split them first on the SPLIT screen.

### SPLIT (stems)
1. The first time, press **DOWNLOAD 290 MB**. The separation models download once into `%APPDATA%\BeatsByBony\models`. (**MODELS FOLDER** opens it.)
2. Load a song on the GRAB screen (or drop it on the plugin).
3. Press **SPLIT IT!**. A full song takes several minutes depending on your computer; **QUICK TEST 30s** only does the first 30 seconds so you can try it fast.
4. Tick **LISTEN** on cassettes to audition them together with **PLAY**, drag a cassette into FL as a WAV, or **SAVE ALL**.
- **Lead / Backing**: the vocals are first pulled out, then split into the main voice and the backing voices. It's good, not perfect; stacked harmonies close to the lead are the hard case.

### BOOST
Press **POWER**, pick A (balanced), B (bass heavy), C (bright), or D (loud and crunchy), then turn the big knob until it hits. **OUTPUT** trims the level afterwards.

---

## Honest notes
- Built and checked on Linux (note detection, stem separation, booster and all three screens were run and screenshotted there). It has **not** been run inside Windows or FL Studio yet, so your first load is the real test.
- Stem quality is "very usable", not "best in the world". The models are older open ones that run on any PC without a graphics card. They can be swapped for better ones later.
- Audio-to-MIDI is a quick-and-clever detector, not magic. Expect to click away a few wrong notes.
- Licences: the plugin framework (JUCE) is free for personal and small-revenue use but needs a paid licence for bigger commercial sales; the ONNX Runtime library is MIT; the separation models come from the open-source Ultimate Vocal Remover project. Check these before you sell the plugin or stems made with it.
