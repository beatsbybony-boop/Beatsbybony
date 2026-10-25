#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace bony
{
struct PreviewEvent { double time; bool on; int midi; float velocity; };
using PreviewEvents = std::vector<PreviewEvent>;

/** Tiny polyphonic synth used to audition the grabbed notes inside the plugin. */
class PreviewSynth
{
public:
    void prepare (double sampleRate) { sr = sampleRate; allOff(); }
    void setSound (int s) { sound = s; }

    void setEvents (std::shared_ptr<const PreviewEvents> e) { events = std::move (e); }

    void seek (double seconds)
    {
        allOff();
        next = 0;
        if (events)
            next = (size_t) (std::lower_bound (events->begin(), events->end(), seconds,
                                               [] (const PreviewEvent& ev, double t) { return ev.time < t; }) - events->begin());
    }

    void audition (int midi, float vel = 0.9f) { startVoice (midi, vel, (int) (sr * 0.45)); }

    /** Adds audio to L/R. startSec is the transport time of the first sample. */
    void render (float* L, float* R, int n, double startSec, bool followEvents, float volume)
    {
        int done = 0;
        while (done < n)
        {
            int chunk = n - done;
            if (followEvents && events && next < events->size())
            {
                const double evT = (*events)[next].time;
                const int at = (int) std::ceil ((evT - startSec) * sr) - done;
                if (at <= 0)
                {
                    const auto& ev = (*events)[next++];
                    if (ev.on) startVoice (ev.midi, ev.velocity, -1); else releaseVoice (ev.midi);
                    continue;
                }
                chunk = std::min (chunk, at);
            }
            renderVoices (L + done, R + done, chunk, volume);
            done += chunk;
        }
    }

    void allOff() { for (auto& v : voices) v.state = Voice::Off; }
    void releaseAll() { for (auto& v : voices) if (v.state != Voice::Off) v.state = Voice::Release; }

private:
    struct Voice
    {
        enum State { Off, Attack, Sustain, Release } state = Off;
        int midi = 60; double phase = 0.0, inc = 0.0; float vel = 0.8f, env = 0.0f, t = 0.0f, lp = 0.0f;
        int autoRelease = -1; unsigned long long age = 0;
    };

    void startVoice (int midi, float vel, int autoRel)
    {
        Voice* v = nullptr;
        for (auto& c : voices) if (c.state == Voice::Off) { v = &c; break; }
        if (! v) { v = &voices[0]; for (auto& c : voices) if (c.age < v->age) v = &c; }
        v->state = Voice::Attack; v->midi = midi; v->phase = 0.0; v->vel = vel; v->env = 0.0f; v->t = 0.0f; v->lp = 0.0f;
        v->inc = 440.0 * std::pow (2.0, (midi - 69) / 12.0) / sr;
        v->autoRelease = autoRel; v->age = ++ageCounter;
    }

    void releaseVoice (int midi)
    {
        for (auto& v : voices) if (v.state != Voice::Off && v.state != Voice::Release && v.midi == midi && v.autoRelease < 0) { v.state = Voice::Release; return; }
    }

    static float polyBlep (double t, double dt)
    {
        if (t < dt)       { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    void renderVoices (float* L, float* R, int n, float volume)
    {
        const float atk = 1.0f - std::exp (-1.0f / (float) (sr * 0.004));
        const float rel = std::exp (-1.0f / (float) (sr * 0.09));
        for (auto& v : voices)
        {
            if (v.state == Voice::Off) continue;
            const float gain = 0.2f * volume * (0.25f + 0.75f * v.vel);
            for (int i = 0; i < n; ++i)
            {
                float s = 0.0f;
                const double ph = v.phase;
                switch (sound)
                {
                    case 0: // keys: soft harmonic stack with a decaying body
                        s = (float) (std::sin (2.0 * M_PI * ph) + 0.35 * std::sin (4.0 * M_PI * ph) * std::exp (-v.t * 6.0f)
                                     + 0.12 * std::sin (6.0 * M_PI * ph) * std::exp (-v.t * 9.0f));
                        s *= 0.55f + 0.45f * std::exp (-v.t * 2.2f);
                        break;
                    case 1: s = (float) (2.0 * ph - 1.0) - polyBlep (ph, v.inc); v.lp += 0.35f * (s - v.lp); s = v.lp; break;
                    case 2: { float q = ph < 0.5 ? 1.0f : -1.0f; q += polyBlep (ph, v.inc); q -= polyBlep (std::fmod (ph + 0.5, 1.0), v.inc);
                              v.lp += 0.4f * (q - v.lp); s = v.lp * 0.8f; break; }
                    default: s = (float) std::sin (2.0 * M_PI * ph); break;
                }
                v.phase += v.inc; if (v.phase >= 1.0) v.phase -= 1.0;
                v.t += 1.0f / (float) sr;

                if (v.autoRelease >= 0 && --v.autoRelease == 0) v.state = Voice::Release;
                if (v.state == Voice::Attack) { v.env += (1.0f - v.env) * atk; if (v.env > 0.995f) v.state = Voice::Sustain; }
                else if (v.state == Voice::Release) { v.env *= rel; if (v.env < 0.0005f) { v.state = Voice::Off; break; } }

                const float o = s * v.env * gain;
                L[i] += o; R[i] += o;
            }
        }
    }

    double sr = 44100.0;
    int sound = 0;
    std::array<Voice, 20> voices;
    std::shared_ptr<const PreviewEvents> events;
    size_t next = 0;
    unsigned long long ageCounter = 0;
};
}
