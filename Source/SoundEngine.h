// Kreisel – built-in sound generator: synthesised drum kit (GM note mapping) and simple synth voices,
// so Kreisel makes sound on its own (standalone, or without routing MIDI to other instruments).
#pragma once
#include "Model.h"
#include <array>
#include <cmath>

namespace kr
{
class SoundEngine
{
public:
    void prepare (double sampleRate);
    void reset();                     // silence immediately
    void releaseAll();                // let everything ring out (transport stop, loop jump)

    // sound = concrete Sound (not sAuto / sOff); level 0..1
    void noteOn (int track, int sound, int note, int velocity, float level);
    void noteOff (int track, int note);

    // adds the voices of one track (or all with track = -1) into left/right; no limiting
    void render (float* left, float* right, int numSamples, int track = -1);

    float masterGain = 1.0f;
    int activeVoices() const;

private:
    enum VoiceType { vKick, vSnare, vClap, vRim, vHatClosed, vHatOpen, vTom, vCrash, vRide, vCowbell, vPerc, vShaker,
                     vTamb, vWood, vTriangle, vAcid, vSub, vSaw, vSquare, vPluck, vPad, vKeys, vBell, vOrgan };

    struct Svf
    {
        float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;
        void set (float fc, float q, float sr);
        float lp (float x) { tick (x); return v2; }
        float bp (float x) { tick (x); return v1; }
        float hp (float x) { tick (x); return x - k * v1 - v2; }
    private:
        float v1 = 0, v2 = 0;
        void tick (float x)
        {
            const float v3 = x - ic2;
            v1 = a1 * ic1 + a2 * v3;
            v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2 * v1 - ic1;
            ic2 = 2 * v2 - ic2;
        }
    };

    struct Voice
    {
        bool active = false, released = false, drum = true;
        int type = 0, track = 0, note = 0;
        float vel = 1, gain = 1, panL = 0.707f, panR = 0.707f, tune = 1, freq = 100;
        double t = 0, relT = 0;
        float relLevel = 0, lastEnv = 0, fadeOut = 0; // fadeOut > 0: choke
        double ph[8] {};
        Svf f1, f2;
        juce::uint32 seed = 1;
        juce::int64 age = 0;
        int filterCounter = 0;
    };

    void start (Voice& v, int type, int track, int note, float vel, float gain, float tune);
    float noise (Voice& v);
    float adsr (Voice& v, float a, float d, float s, float r);
    bool renderVoice (Voice& v, float* left, float* right, int n); // false = finished

    static constexpr int kVoices = 48;
    std::array<Voice, kVoices> voices;
    double sr = 44100.0;
    float isr = 1.0f / 44100.0f;
    juce::int64 counter = 0;
};
} // namespace kr
