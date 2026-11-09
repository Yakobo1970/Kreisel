#include "SoundEngine.h"

namespace kr
{
namespace
{
constexpr float twoPi = 6.283185307179586f;

inline float blep (double t, double dt)
{
    if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
    return 0.0f;
}
inline float saw (double ph, double dt) { return (float) (2.0 * ph - 1.0) - blep (ph, dt); }
inline float square (double ph, double dt)
{
    double p2 = ph + 0.5; if (p2 >= 1.0) p2 -= 1.0;
    return (ph < 0.5 ? 1.0f : -1.0f) + blep (ph, dt) - blep (p2, dt);
}
inline void adv (double& ph, double inc) { ph += inc; if (ph >= 1.0) ph -= std::floor (ph); }
inline float mtof (float note) { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }
inline float fastTanh (float x) { const float x2 = x * x; return juce::jlimit (-1.0f, 1.0f, x * (27.0f + x2) / (27.0f + 9.0f * x2)); }

const float metalFreqs[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
} // namespace

void SoundEngine::Svf::set (float fc, float q, float sampleRate)
{
    fc = juce::jlimit (20.0f, sampleRate * 0.45f, fc);
    const float g = std::tan (juce::MathConstants<float>::pi * fc / sampleRate);
    k = 1.0f / juce::jmax (0.05f, q);
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

void SoundEngine::prepare (double sampleRate)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    isr = (float) (1.0 / sr);
    reset();
}

void SoundEngine::reset()
{
    for (auto& v : voices) v.active = false;
}

void SoundEngine::releaseAll()
{
    for (auto& v : voices)
        if (v.active && ! v.released && ! v.drum)
        {
            v.released = true;
            v.relT = v.t;
            v.relLevel = v.lastEnv;
        }
}

int SoundEngine::activeVoices() const
{
    int n = 0;
    for (auto& v : voices) n += v.active ? 1 : 0;
    return n;
}

float SoundEngine::noise (Voice& v)
{
    v.seed ^= v.seed << 13; v.seed ^= v.seed >> 17; v.seed ^= v.seed << 5;
    return (float) (v.seed & 0xffffff) / 8388608.0f - 1.0f;
}

float SoundEngine::adsr (Voice& v, float a, float d, float s, float r)
{
    const float t = (float) v.t;
    float e;
    if (! v.released)
    {
        if (t < a) e = t / a;
        else if (t < a + d) e = 1.0f - (1.0f - s) * ((t - a) / d);
        else e = s;
        v.lastEnv = e;
        return e;
    }
    e = v.relLevel * (1.0f - (float) (v.t - v.relT) / r);
    return juce::jmax (0.0f, e);
}

void SoundEngine::start (Voice& v, int type, int track, int note, float vel, float gain, float tune)
{
    const auto age = ++counter;
    const auto seed = v.seed;
    v = Voice();
    v.active = true;
    v.type = type; v.track = track; v.note = note;
    v.vel = vel; v.gain = gain; v.tune = tune;
    v.age = age;
    v.seed = seed * 1664525u + 1013904223u + (juce::uint32) note * 7919u + 1u;
    v.drum = type <= vTriangle;
    v.freq = mtof ((float) note);
    float pan = 0.0f;
    switch (type)
    {
        case vHatClosed: case vHatOpen: pan = 0.25f; break;
        case vRide: pan = -0.25f; break;
        case vCrash: pan = -0.2f; break;
        case vPerc: pan = (note % 2 == 0) ? -0.3f : 0.3f; break;
        case vShaker: case vTamb: pan = 0.35f; break;
        case vCowbell: case vWood: pan = -0.3f; break;
        case vTom: pan = juce::jlimit (-0.4f, 0.4f, (note - 45) * 0.06f); break;
        default: break;
    }
    v.panL = std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
    v.panR = std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);

    // filters that do not move
    switch (type)
    {
        case vSnare: v.f1.set (1800.0f, 0.7f, (float) sr); break;
        case vClap: v.f1.set (1250.0f, 1.3f, (float) sr); break;
        case vHatClosed: v.f1.set (9000.0f, 0.8f, (float) sr); v.f2.set (6500.0f, 0.7f, (float) sr); break;
        case vHatOpen: v.f1.set (8500.0f, 0.8f, (float) sr); v.f2.set (6000.0f, 0.7f, (float) sr); break;
        case vTamb: v.f1.set (8000.0f, 0.7f, (float) sr); break;
        case vCrash: v.f1.set (5000.0f, 0.7f, (float) sr); break;
        case vRide: v.f1.set (6800.0f, 1.2f, (float) sr); break;
        case vCowbell: v.f1.set (2600.0f, 2.0f, (float) sr); break;
        case vPerc: v.f1.set (2200.0f, 1.0f, (float) sr); break;
        case vShaker: v.f1.set (6500.0f, 1.1f, (float) sr); break;
        case vSub: break;
        default: break;
    }
}

void SoundEngine::noteOn (int track, int sound, int note, int velocity, float level)
{
    if (sound <= sAuto || level <= 0.0f) return;
    const float vel = juce::jlimit (1, 127, velocity) / 127.0f;
    int type = vKick;
    float tune = 1.0f;
    auto rel = [&] (int ref) { tune = juce::jlimit (0.5f, 2.0f, std::pow (2.0f, (note - ref) / 12.0f)); };

    switch (sound)
    {
        case sKit:
            switch (note)
            {
                case 35: case 36: type = vKick; tune = note == 35 ? 0.85f : 1.0f; break;
                case 37: type = vRim; break;
                case 38: case 40: type = vSnare; tune = note == 40 ? 1.1f : 1.0f; break;
                case 39: type = vClap; break;
                case 41: case 43: case 45: case 47: case 48: case 50: type = vTom; break;
                case 42: case 44: type = vHatClosed; tune = note == 44 ? 0.9f : 1.0f; break;
                case 46: type = vHatOpen; break;
                case 49: case 52: case 55: case 57: type = vCrash; tune = note == 55 ? 1.3f : (note == 52 ? 0.8f : 1.0f); break;
                case 51: case 53: case 59: type = vRide; tune = note == 53 ? 1.25f : 1.0f; break;
                case 54: type = vTamb; break;
                case 56: case 67: case 68: type = vCowbell; tune = note == 56 ? 1.0f : (note == 67 ? 1.5f : 1.3f); break;
                case 69: case 70: case 73: case 74: case 82: type = vShaker; tune = note == 82 ? 1.0f : 0.85f; break;
                case 75: case 76: case 77: type = vWood; break;
                case 80: case 81: type = vTriangle; break;
                default: type = vPerc; break;
            }
            break;
        case sKick: type = vKick; rel (36); break;
        case sSnare: type = vSnare; rel (38); break;
        case sClap: type = vClap; rel (39); break;
        case sRim: type = vRim; rel (37); break;
        case sHatClosed: type = vHatClosed; rel (42); break;
        case sHatOpen: type = vHatOpen; rel (46); break;
        case sTom: type = vTom; break;
        case sCymbal: type = vCrash; rel (49); break;
        case sCowbell: type = vCowbell; rel (56); break;
        case sPerc: type = vPerc; break;
        case sShaker: type = vShaker; rel (82); break;
        case sAcid: type = vAcid; break;
        case sSub: type = vSub; break;
        case sSawLead: type = vSaw; break;
        case sSquareLead: type = vSquare; break;
        case sPluck: type = vPluck; break;
        case sPad: type = vPad; break;
        case sKeys: type = vKeys; break;
        case sBell: type = vBell; break;
        case sOrgan: type = vOrgan; break;
        default: return;
    }

    // choke: closed / pedal hat cuts an open hat of the same track; same note on a track retriggers
    for (auto& v : voices)
    {
        if (! v.active || v.track != track) continue;
        const bool sameNote = v.note == note && v.type == type;
        const bool choke = type == vHatClosed && v.type == vHatOpen;
        if ((sameNote || choke) && v.fadeOut == 0.0f)
        {
            if (v.drum || choke) { v.fadeOut = 1.0f; v.relT = v.t; }
            else if (! v.released) { v.released = true; v.relT = v.t; v.relLevel = v.lastEnv; }
        }
    }

    // free voice, else steal the oldest released, else the oldest
    Voice* slot = nullptr;
    for (auto& v : voices) if (! v.active) { slot = &v; break; }
    if (slot == nullptr)
        for (auto& v : voices) if (v.released && (slot == nullptr || v.age < slot->age)) slot = &v;
    if (slot == nullptr)
        for (auto& v : voices) if (slot == nullptr || v.age < slot->age) slot = &v;

    start (*slot, type, track, note, vel, level, tune);
}

void SoundEngine::noteOff (int track, int note)
{
    for (auto& v : voices)
        if (v.active && ! v.drum && ! v.released && v.track == track && v.note == note)
        {
            v.released = true;
            v.relT = v.t;
            v.relLevel = v.lastEnv;
        }
}

void SoundEngine::render (float* left, float* right, int numSamples, int track)
{
    if (numSamples <= 0) return;
    for (auto& v : voices)
        if (v.active && (track < 0 || v.track == track) && ! renderVoice (v, left, right, numSamples))
            v.active = false;
}

bool SoundEngine::renderVoice (Voice& v, float* left, float* right, int n)
{
    const float fs = (float) sr;
    const double dt = 1.0 / sr;
    const float g = v.gain * (0.35f + 0.65f * v.vel);
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) v.t;
        float out = 0.0f, outR = 0.0f;
        bool stereo = false, done = false;

        switch (v.type)
        {
            case vKick:
            {
                const float f = (46.0f + 120.0f * std::exp (-t / 0.032f)) * v.tune;
                adv (v.ph[0], f * dt);
                const float body = std::sin (twoPi * (float) v.ph[0]) * std::exp (-t / 0.30f);
                const float click = noise (v) * std::exp (-t / 0.0025f) * 0.35f;
                out = fastTanh (1.7f * (body + click)) * 0.95f;
                done = t > 1.3f;
                break;
            }
            case vSnare:
            {
                const float f = 185.0f * v.tune * (1.0f + 0.25f * std::exp (-t / 0.01f));
                adv (v.ph[0], f * dt);
                adv (v.ph[1], f * 1.72 * dt);
                const float tone = (std::sin (twoPi * (float) v.ph[0]) + 0.4f * std::sin (twoPi * (float) v.ph[1])) * std::exp (-t / 0.055f);
                const float nz = v.f1.hp (noise (v)) * std::exp (-t / 0.16f);
                out = 0.4f * tone + 0.6f * nz;
                done = t > 0.7f;
                break;
            }
            case vClap:
            {
                float env = 0.0f;
                for (int k = 0; k < 3; ++k)
                {
                    const float tk = t - k * 0.0105f;
                    if (tk >= 0) env += std::exp (-tk / 0.0045f);
                }
                if (t > 0.021f) env += 0.7f * std::exp (-(t - 0.021f) / 0.13f);
                out = v.f1.bp (noise (v)) * env * 1.25f;
                done = t > 0.7f;
                break;
            }
            case vRim:
            {
                adv (v.ph[0], 1650.0 * v.tune * dt);
                adv (v.ph[1], 470.0 * v.tune * dt);
                out = std::sin (twoPi * (float) v.ph[0]) * std::exp (-t / 0.011f)
                      + 0.6f * std::sin (twoPi * (float) v.ph[1]) * std::exp (-t / 0.02f)
                      + noise (v) * std::exp (-t / 0.0015f) * 0.3f;
                out *= 0.7f;
                done = t > 0.2f;
                break;
            }
            case vHatClosed: case vHatOpen: case vCrash: case vRide: case vTamb:
            {
                const float mul = v.type == vCrash ? 1.45f : (v.type == vRide ? 2.1f : (v.type == vTamb ? 2.6f : 1.6f));
                float m = 0.0f;
                for (int k = 0; k < 6; ++k)
                {
                    adv (v.ph[k], metalFreqs[k] * mul * v.tune * dt);
                    m += v.ph[k] < 0.5 ? 1.0f : -1.0f;
                }
                m *= 0.16f;
                const float nz = noise (v);
                float env = 0.0f;
                switch (v.type)
                {
                    case vHatClosed: env = std::exp (-t / 0.042f); out = v.f2.hp (v.f1.bp (m + 0.5f * nz)) * env * 0.9f; done = t > 0.35f; break;
                    case vHatOpen: env = std::exp (-t / 0.33f); out = v.f2.hp (v.f1.bp (m + 0.5f * nz)) * env * 0.8f; done = t > 2.0f; break;
                    case vCrash: env = std::exp (-t / 1.1f) * (1.0f - 0.5f * std::exp (-t / 0.01f)); out = v.f1.hp (m + 0.8f * nz) * env * 0.55f; done = t > 4.5f; break;
                    case vRide:
                        adv (v.ph[6], 3100.0 * v.tune * dt);
                        env = std::exp (-t / 0.55f);
                        out = (v.f1.bp (m + 0.3f * nz) * 0.7f + 0.18f * std::sin (twoPi * (float) v.ph[6])) * env;
                        done = t > 2.8f;
                        break;
                    default: // tambourine
                        env = std::exp (-t / 0.11f) * (0.75f + 0.25f * std::sin (twoPi * 28.0f * t));
                        out = v.f1.hp (0.6f * m + nz) * env * 0.6f;
                        done = t > 0.6f;
                        break;
                }
                break;
            }
            case vTom:
            {
                const float base = 100.0f * std::pow (2.0f, (v.note - 45) / 8.0f);
                adv (v.ph[0], base * (1.0f + 0.6f * std::exp (-t / 0.05f)) * dt);
                out = std::sin (twoPi * (float) v.ph[0]) * std::exp (-t / 0.32f) + noise (v) * std::exp (-t / 0.004f) * 0.2f;
                out = fastTanh (1.3f * out) * 0.85f;
                done = t > 1.4f;
                break;
            }
            case vCowbell:
            {
                adv (v.ph[0], 540.0 * v.tune * dt);
                adv (v.ph[1], 800.0 * v.tune * dt);
                const float m = (v.ph[0] < 0.5 ? 1.0f : -1.0f) + (v.ph[1] < 0.5 ? 1.0f : -1.0f);
                out = v.f1.bp (m) * (0.6f * std::exp (-t / 0.015f) + 0.4f * std::exp (-t / 0.11f)) * 0.45f;
                done = t > 0.7f;
                break;
            }
            case vPerc:
            {
                const float f = juce::jlimit (110.0f, 1400.0f, v.freq) * (1.0f + 0.15f * std::exp (-t / 0.02f));
                adv (v.ph[0], f * dt);
                out = std::sin (twoPi * (float) v.ph[0]) * std::exp (-t / 0.16f)
                      + v.f1.bp (noise (v)) * std::exp (-t / 0.005f) * 0.4f;
                out *= 0.8f;
                done = t > 0.8f;
                break;
            }
            case vShaker:
            {
                const float env = t < 0.006f ? t / 0.006f : std::exp (-(t - 0.006f) / 0.05f);
                out = v.f1.bp (noise (v)) * env * 0.9f;
                done = t > 0.35f;
                break;
            }
            case vWood:
            {
                const float f = v.note == 75 ? 2500.0f : 900.0f * std::pow (2.0f, (v.note - 76) / 12.0f);
                adv (v.ph[0], f * dt);
                adv (v.ph[1], f * 2.7 * dt);
                out = (std::sin (twoPi * (float) v.ph[0]) + 0.3f * std::sin (twoPi * (float) v.ph[1])) * std::exp (-t / 0.025f) * 0.7f;
                done = t > 0.25f;
                break;
            }
            case vTriangle:
            {
                adv (v.ph[0], 2600.0 * dt);
                adv (v.ph[1], 3900.0 * dt);
                out = (std::sin (twoPi * (float) v.ph[0]) + 0.5f * std::sin (twoPi * (float) v.ph[1])) * std::exp (-t / 0.7f) * 0.35f;
                done = t > 3.5f;
                break;
            }
            // ------------------------------------------------------------- melodic voices
            case vAcid:
            {
                const double inc = v.freq * dt;
                adv (v.ph[0], inc);
                if ((v.filterCounter++ & 7) == 0)
                    v.f1.set (170.0f + (900.0f + 2600.0f * v.vel) * std::exp (-t / 0.17f), 3.2f, fs);
                const float env = adsr (v, 0.002f, 0.25f, 0.55f, 0.05f);
                out = fastTanh (1.6f * v.f1.lp (saw (v.ph[0], inc))) * env * 0.55f;
                done = v.released && env <= 0.0f;
                break;
            }
            case vSub:
            {
                adv (v.ph[0], v.freq * dt);
                const float env = adsr (v, 0.003f, 0.1f, 0.9f, 0.07f);
                const float p = twoPi * (float) v.ph[0];
                out = (std::sin (p) + 0.18f * std::sin (2.0f * p)) * env * 0.65f;
                done = v.released && env <= 0.0f;
                break;
            }
            case vSaw:
            {
                const double i1 = v.freq * 1.004 * dt, i2 = v.freq * 0.996 * dt;
                adv (v.ph[0], i1); adv (v.ph[1], i2);
                if ((v.filterCounter++ & 7) == 0)
                {
                    const float fc = 1700.0f + 1800.0f * std::exp (-t / 0.25f) * v.vel;
                    v.f1.set (fc, 0.9f, fs); v.f2.set (fc, 0.9f, fs);
                }
                const float env = adsr (v, 0.004f, 0.3f, 0.75f, 0.14f);
                out = v.f1.lp (saw (v.ph[0], i1)) * env * 0.36f;
                outR = v.f2.lp (saw (v.ph[1], i2)) * env * 0.36f;
                stereo = true;
                done = v.released && env <= 0.0f;
                break;
            }
            case vSquare:
            {
                const double i1 = v.freq * dt, i2 = v.freq * 0.5 * dt;
                adv (v.ph[0], i1); adv (v.ph[1], i2);
                if (v.filterCounter++ == 0) v.f1.set (2800.0f, 0.8f, fs);
                const float env = adsr (v, 0.003f, 0.2f, 0.7f, 0.1f);
                out = v.f1.lp (square (v.ph[0], i1) + 0.4f * square (v.ph[1], i2)) * env * 0.28f;
                done = v.released && env <= 0.0f;
                break;
            }
            case vPluck:
            {
                const double i1 = v.freq * dt, i2 = v.freq * 1.003 * dt;
                adv (v.ph[0], i1); adv (v.ph[1], i2);
                if ((v.filterCounter++ & 7) == 0)
                    v.f1.set (350.0f + 4200.0f * v.vel * std::exp (-t / 0.09f), 1.2f, fs);
                const float env = adsr (v, 0.001f, 0.45f, 0.0f, 0.12f);
                out = v.f1.lp (saw (v.ph[0], i1) + saw (v.ph[1], i2)) * env * 0.3f;
                done = (v.released && env <= 0.0f) || t > 0.5f;
                break;
            }
            case vPad:
            {
                const double i1 = v.freq * dt, i2 = v.freq * 1.007 * dt, i3 = v.freq * 0.993 * dt;
                adv (v.ph[0], i1); adv (v.ph[1], i2); adv (v.ph[2], i3); adv (v.ph[3], v.freq * 0.5 * dt);
                if ((v.filterCounter++ & 15) == 0)
                {
                    const float fc = 1100.0f + 350.0f * std::sin (twoPi * 0.25f * t);
                    v.f1.set (fc, 0.7f, fs); v.f2.set (fc, 0.7f, fs);
                }
                const float env = adsr (v, 0.35f, 0.8f, 0.75f, 0.7f);
                const float sub = 0.3f * std::sin (twoPi * (float) v.ph[3]);
                out = v.f1.lp (saw (v.ph[0], i1) + saw (v.ph[1], i2) * 0.8f + sub) * env * 0.17f;
                outR = v.f2.lp (saw (v.ph[0], i1) + saw (v.ph[2], i3) * 0.8f + sub) * env * 0.17f;
                stereo = true;
                done = v.released && env <= 0.0f;
                break;
            }
            case vKeys:
            {
                adv (v.ph[0], v.freq * dt);
                adv (v.ph[1], v.freq * 4.0 * dt);
                const float idx = 2.0f * std::exp (-t / 0.35f) + 0.3f;
                const float mod = idx * std::sin (twoPi * (float) v.ph[0]);
                const float env = adsr (v, 0.002f, 1.6f, 0.25f, 0.25f);
                out = (std::sin (twoPi * (float) v.ph[0] + mod) + 0.15f * std::sin (twoPi * (float) v.ph[1]) * std::exp (-t / 0.05f)) * env * 0.32f;
                done = v.released && env <= 0.0f;
                break;
            }
            case vBell:
            {
                adv (v.ph[0], v.freq * dt);
                adv (v.ph[1], v.freq * 3.5 * dt);
                const float mod = 2.2f * std::exp (-t / 0.8f) * std::sin (twoPi * (float) v.ph[1]);
                float env = std::exp (-t / 1.5f);
                if (v.released) env *= juce::jmax (0.0f, 1.0f - (float) (v.t - v.relT) / 0.6f);
                v.lastEnv = env;
                out = std::sin (twoPi * (float) v.ph[0] + mod) * env * 0.28f;
                done = env <= 0.0005f;
                break;
            }
            case vOrgan:
            {
                const float h[5] = { 1.0f, 0.6f, 0.4f, 0.25f, 0.15f };
                const double mul[5] = { 1, 2, 3, 4, 6 };
                float s = 0.0f;
                for (int k = 0; k < 5; ++k)
                {
                    adv (v.ph[k], v.freq * mul[k] * dt);
                    s += h[k] * std::sin (twoPi * (float) v.ph[k]);
                }
                const float env = adsr (v, 0.006f, 0.05f, 1.0f, 0.06f);
                out = s * env * 0.16f * (1.0f + 0.08f * std::sin (twoPi * 6.0f * t));
                done = v.released && env <= 0.0f;
                break;
            }
            default: done = true; break;
        }

        if (v.fadeOut > 0.0f)
        {
            const float f = 1.0f - (float) (v.t - v.relT) / 0.012f;
            if (f <= 0.0f) done = true;
            out *= juce::jmax (0.0f, f);
            outR *= juce::jmax (0.0f, f);
        }
        if (! stereo) outR = out;
        left[i] += out * g * v.panL * 1.414f;
        right[i] += outR * g * v.panR * 1.414f;
        v.t += dt;
        if (done) return false;
    }
    return true;
}
} // namespace kr
