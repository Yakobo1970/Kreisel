// Kreisel – euclidean MIDI sequencer
// Shared definitions: tracks, modulation targets, divisions, scales, chords.
#pragma once
#include <juce_core/juce_core.h>
#include <array>

namespace kr
{
constexpr int kTracks = 8;
constexpr int kMaxSteps = 64;
constexpr int kMaxChordNotes = 6;

// The nine per-track parameters that own a modulation lane (8 x 9 = 72 lanes).
enum Target { tSteps, tBeats, tRotate, tChance, tVelocity, tSwing, tPitch, tChord, tClock, kTargets };

inline const char* const targetNames[kTargets] = { "Steps", "Beats", "Rotate", "Chance", "Velocity",
                                                   "Swing", "Pitch", "Chord", "Clock" };
// How far a lane at +/-100 % (depth 1) moves its target.
inline const int targetRange[kTargets] = { 63, 64, 63, 100, 127, 75, 24, 18, 17 };

struct Division { const char* name; double quarters; };
inline const Division divisions[] = {
    { "4/1", 16.0 },  { "2/1", 8.0 },   { "1/1", 4.0 },       { "1/2.", 3.0 },  { "1/2", 2.0 },
    { "1/2T", 4.0 / 3 }, { "1/4.", 1.5 }, { "1/4", 1.0 },     { "1/4T", 2.0 / 3 }, { "1/8.", 0.75 },
    { "1/8", 0.5 },   { "1/8T", 1.0 / 3 }, { "1/16.", 0.375 }, { "1/16", 0.25 }, { "1/16T", 1.0 / 6 },
    { "1/32", 0.125 }, { "1/32T", 1.0 / 12 }, { "1/64", 0.0625 }
};
constexpr int kNumDivisions = (int) (sizeof (divisions) / sizeof (divisions[0]));
constexpr int kDefaultDivision = 13; // 1/16

struct Scale { const char* name; int count; int steps[12]; };
inline const Scale scales[] = {
    { "Chromatisch", 12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
    { "Dur", 7, { 0, 2, 4, 5, 7, 9, 11 } },
    { "Moll", 7, { 0, 2, 3, 5, 7, 8, 10 } },
    { "Harmonisch Moll", 7, { 0, 2, 3, 5, 7, 8, 11 } },
    { "Melodisch Moll", 7, { 0, 2, 3, 5, 7, 9, 11 } },
    { "Dorisch", 7, { 0, 2, 3, 5, 7, 9, 10 } },
    { "Phrygisch", 7, { 0, 1, 3, 5, 7, 8, 10 } },
    { "Lydisch", 7, { 0, 2, 4, 6, 7, 9, 11 } },
    { "Mixolydisch", 7, { 0, 2, 4, 5, 7, 9, 10 } },
    { "Lokrisch", 7, { 0, 1, 3, 5, 6, 8, 10 } },
    { "Phrygisch Dominant", 7, { 0, 1, 4, 5, 7, 8, 10 } },
    { "Ungarisch Moll", 7, { 0, 2, 3, 6, 7, 8, 11 } },
    { "Dur-Pentatonik", 5, { 0, 2, 4, 7, 9 } },
    { "Moll-Pentatonik", 5, { 0, 3, 5, 7, 10 } },
    { "Blues", 6, { 0, 3, 5, 6, 7, 10 } },
    { "Ganzton", 6, { 0, 2, 4, 6, 8, 10 } }
};
constexpr int kNumScales = (int) (sizeof (scales) / sizeof (scales[0]));

inline const char* const noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

inline juce::String midiNoteName (int n) { return juce::String (noteNames[(n % 12 + 12) % 12]) + juce::String (n / 12 - 2); }

// Chords. "scale" chords are stacked from scale degrees (diatonic), the rest are fixed intervals.
struct Chord { const char* name; bool scale; int count; int iv[kMaxChordNotes]; };
inline const Chord chords[] = {
    { "Aus", false, 1, { 0 } },
    { "Skala Dreiklang", true, 3, { 0, 2, 4 } },
    { "Skala Septim", true, 4, { 0, 2, 4, 6 } },
    { "Skala None", true, 5, { 0, 2, 4, 6, 8 } },
    { "Skala Sus2", true, 3, { 0, 1, 4 } },
    { "Skala Sus4", true, 3, { 0, 3, 4 } },
    { "Skala Quarten", true, 3, { 0, 3, 6 } },
    { "Quinte", false, 2, { 0, 7 } },
    { "Oktave", false, 2, { 0, 12 } },
    { "Dur", false, 3, { 0, 4, 7 } },
    { "Moll", false, 3, { 0, 3, 7 } },
    { "Maj7", false, 4, { 0, 4, 7, 11 } },
    { "m7", false, 4, { 0, 3, 7, 10 } },
    { "7", false, 4, { 0, 4, 7, 10 } },
    { "Vermindert", false, 3, { 0, 3, 6 } },
    { "Übermäßig", false, 3, { 0, 4, 8 } },
    { "Sus2", false, 3, { 0, 2, 7 } },
    { "Sus4", false, 3, { 0, 5, 7 } },
    { "Add9", false, 4, { 0, 4, 7, 14 } }
};
constexpr int kNumChords = (int) (sizeof (chords) / sizeof (chords[0]));

inline const char* const laneDirNames[] = { "Vorwärts", "Rückwärts", "Ping-Pong", "Zufall" };
constexpr int kNumLaneDirs = 4;

inline const char* const visualNames[] = { "Band", "Ringe", "Polygon", "Speichen", "Raster",
                                           "Puls", "Spirale", "Dichte", "Blüte" };
constexpr int kNumVisuals = 9;

// ------------------------------------------------------------------ built-in sounds
enum Sound { sOff, sAuto, sKit, sKick, sSnare, sClap, sRim, sHatClosed, sHatOpen, sTom, sCymbal, sCowbell, sPerc, sShaker,
             sAcid, sSub, sSawLead, sSquareLead, sPluck, sPad, sKeys, sBell, sOrgan, kNumSounds };
inline const char* const soundNames[kNumSounds] = {
    "Aus (nur MIDI)", "Automatisch", "Drum-Kit (nach Note)", "Kick", "Snare", "Clap", "Rimshot", "Hi-Hat zu", "Hi-Hat offen",
    "Tom", "Becken", "Cowbell", "Perkussion", "Shaker", "Acid-Bass", "Sub-Bass", "Saw-Lead", "Square-Lead", "Pluck",
    "Fläche", "Keys", "Glocke", "Orgel" };

// ------------------------------------------------------------------ runtime snapshot
struct TrackSettings
{
    bool active = true, mute = false, solo = false;
    int steps = 16, beats = 4, rotate = 0;
    float chance = 100.0f;   // %
    int velocity = 100;
    float swing = 0.0f;      // % of a step (0..75)
    int pitch = 36;
    int chord = 0;
    int clock = kDefaultDivision;
    float gate = 50.0f;      // % of a step (5..200)
    int channel = 1;
    int sound = 1;           // soundNames, 1 = automatic
    float level = 80.0f;     // % internal sound level
    juce::String name;       // custom track name, empty = automatic (never touched on the audio thread)
};

struct LaneSettings
{
    bool on = false;
    int length = 16;      // 2..64
    int dir = 0;          // laneDirNames
    float depth = 1.0f;   // 0..1
    std::array<float, kMaxSteps> v {}; // -1..+1
};

struct Snapshot
{
    std::array<TrackSettings, kTracks> tracks;
    std::array<std::array<LaneSettings, kTargets>, kTracks> lanes;
    int root = 0, scale = 0;
    bool quantize = false;
};

// Euclidean hit test (Bjorklund-equivalent distribution), position already rotated by caller.
inline bool euclidHit (int pos, int steps, int beats)
{
    if (steps <= 0 || beats <= 0) return false;
    if (beats >= steps) return true;
    return ((pos * beats) % steps) < beats;
}

inline bool patternHit (int pos, int steps, int beats, int rotate)
{
    if (steps <= 0) return false;
    const int r = ((rotate % steps) + steps) % steps;
    return euclidHit (((pos - r) % steps + steps) % steps, steps, beats);
}
// "Automatisch": picks a built-in sound from the MIDI channel (GM drums on 10, chords 1, bass 2, lead 3).
inline int resolveSound (const TrackSettings& t)
{
    if (t.sound != sAuto) return juce::jlimit (0, kNumSounds - 1, t.sound);
    switch (t.channel)
    {
        case 10: return sKit;
        case 2:  return t.pitch < 34 ? sSub : sAcid;
        case 1:  return t.gate >= 120.0f ? sPad : (t.chord > 0 ? sKeys : sPluck);
        case 3:  return t.gate <= 45.0f ? sPluck : sSawLead;
        case 4:  return t.gate >= 120.0f ? sPad : sPluck;
        default: return t.gate >= 120.0f ? sPad : sKeys;
    }
}

inline juce::String gmDrumName (int note)
{
    switch (note)
    {
        case 35: case 36: return "Kick";
        case 37: return "Rimshot";
        case 38: case 40: return "Snare";
        case 39: return "Clap";
        case 41: case 43: return "Tom tief";
        case 45: case 47: return "Tom mittel";
        case 48: case 50: return "Tom hoch";
        case 42: return "Hi-Hat";
        case 44: return "Pedal-Hat";
        case 46: return "Open Hat";
        case 49: case 57: return "Crash";
        case 51: case 59: return "Ride";
        case 52: return "China";
        case 53: return "Ride-Glocke";
        case 54: return "Tamburin";
        case 55: return "Splash";
        case 56: return "Cowbell";
        case 60: case 61: return "Bongo";
        case 62: case 63: case 64: return "Conga";
        case 65: case 66: return "Timbale";
        case 67: case 68: return "Agogo";
        case 69: return "Cabasa";
        case 70: return "Maracas";
        case 73: case 74: return "Guiro";
        case 75: return "Clave";
        case 76: case 77: return "Holzblock";
        case 80: case 81: return "Triangel";
        case 82: return "Shaker";
        default: return "Perkussion";
    }
}

// Display name of a track: custom name, else derived from sound / GM drum note / MIDI channel.
inline juce::String autoTrackName (const TrackSettings& t)
{
    if (t.name.isNotEmpty()) return t.name;
    if (t.sound > sKit) return juce::String::fromUTF8 (soundNames[juce::jlimit (0, kNumSounds - 1, t.sound)]);
    if (t.channel == 10 || t.sound == sKit) return gmDrumName (t.pitch);
    switch (t.channel)
    {
        case 1: return t.chord > 0 ? "Akkorde" : (t.gate >= 120.0f ? juce::String::fromUTF8 ("Fläche") : juce::String ("Melodie"));
        case 2: return "Bass";
        case 3: return "Lead";
        case 4: return "Arp";
        default: return "Kanal " + juce::String (t.channel);
    }
}
} // namespace kr
