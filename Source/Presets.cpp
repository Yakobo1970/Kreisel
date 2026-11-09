// Factory presets – all patterns written for Kreisel (GM drum notes on channel 10 for drum kits,
// melodic tracks on channels 1-4).
#include "Presets.h"
#include <cmath>
#include <cstring>
#include <vector>

namespace kr
{
namespace
{
enum Div { D4_1, D2_1, D1_1, D1_2d, D1_2, D1_2t, D1_4d, D1_4, D1_4t, D1_8d, D1_8, D1_8t, D1_16d, D1_16, D1_16t, D1_32, D1_32t, D1_64 };

struct T
{
    int steps, beats, rotate, pitch, vel; Div clock;
    float chance = 100, swing = 0, gate = 50; int chord = 0, channel = 10;
};

void setTrack (Snapshot& s, int i, const T& t)
{
    auto& tr = s.tracks[(size_t) i];
    tr.active = true; tr.mute = false; tr.solo = false;
    tr.steps = t.steps; tr.beats = t.beats; tr.rotate = t.rotate; tr.pitch = t.pitch;
    tr.velocity = t.vel; tr.clock = (int) t.clock; tr.chance = t.chance; tr.swing = t.swing;
    tr.gate = t.gate; tr.chord = t.chord; tr.channel = t.channel;
}

void lane (Snapshot& s, int track, Target tg, int length, int dir, float depth, std::initializer_list<float> vals)
{
    auto& l = s.lanes[(size_t) track][(size_t) tg];
    l.on = true; l.length = length; l.dir = dir; l.depth = depth;
    int i = 0;
    for (auto v : vals) if (i < kMaxSteps) l.v[(size_t) i++] = v;
    for (int j = i; j < length && i > 0; ++j) l.v[(size_t) j] = l.v[(size_t) (j % i)];
}

void sineLane (Snapshot& s, int track, Target tg, int length, float depth, float cycles = 1.0f)
{
    auto& l = s.lanes[(size_t) track][(size_t) tg];
    l.on = true; l.length = length; l.dir = 0; l.depth = depth;
    for (int i = 0; i < length; ++i)
        l.v[(size_t) i] = std::sin (juce::MathConstants<float>::twoPi * cycles * (float) i / (float) length);
}

Snapshot blank (int root, int scale, bool quant)
{
    Snapshot s;
    for (auto& t : s.tracks) t.active = false;
    s.root = root; s.scale = scale; s.quantize = quant;
    return s;
}

struct Def { const char* cat; const char* name; Snapshot (*make)(); };

Snapshot pTechnoDrive()
{
    auto s = blank (9, 2, false);
    setTrack (s, 0, { 16, 4, 0, 36, 120, D1_16 });
    setTrack (s, 1, { 16, 2, 4, 39, 105, D1_16 });
    setTrack (s, 2, { 16, 4, 2, 46, 95, D1_16, 100, 0, 25 });
    setTrack (s, 3, { 16, 11, 0, 42, 80, D1_16, 85, 8, 20 });
    setTrack (s, 4, { 13, 5, 0, 75, 90, D1_16, 70 });
    setTrack (s, 5, { 32, 3, 7, 49, 100, D1_16, 60 });
    sineLane (s, 3, tVelocity, 16, 0.25f);
    lane (s, 4, tRotate, 8, 0, 0.1f, { 0, 0, 0.5f, 0, 0, 1, 0, 0.5f });
    return s;
}

Snapshot pTechnoHypnotic()
{
    auto s = blank (2, 6, true);
    setTrack (s, 0, { 16, 4, 0, 36, 118, D1_16 });
    setTrack (s, 1, { 16, 3, 2, 42, 85, D1_16, 90, 10, 20 });
    setTrack (s, 2, { 7, 3, 0, 50, 100, D1_16, 100, 0, 40, 0, 1 });
    setTrack (s, 3, { 9, 4, 1, 62, 80, D1_16, 75, 0, 30, 0, 2 });
    setTrack (s, 4, { 5, 2, 0, 38, 100, D1_8, 100, 0, 80, 0, 3 });
    lane (s, 2, tPitch, 8, 2, 0.15f, { 0, 0.3f, 0.6f, 0.3f, 0, -0.3f, 0, 0.6f });
    lane (s, 3, tPitch, 12, 3, 0.2f, { 0, 0.5f, -0.5f, 1, 0, 0.25f });
    sineLane (s, 2, tVelocity, 32, 0.2f);
    return s;
}

Snapshot pHouseGroove()
{
    auto s = blank (5, 1, true);
    setTrack (s, 0, { 16, 4, 0, 36, 115, D1_16 });
    setTrack (s, 1, { 16, 2, 4, 39, 100, D1_16 });
    setTrack (s, 2, { 16, 4, 2, 46, 90, D1_16, 100, 15, 25 });
    setTrack (s, 3, { 16, 9, 1, 42, 70, D1_16, 90, 15, 15 });
    setTrack (s, 4, { 16, 5, 3, 53, 95, D1_16, 100, 15, 60, 2, 1 });
    setTrack (s, 5, { 8, 3, 0, 41, 100, D1_8, 100, 15, 45, 0, 2 });
    lane (s, 4, tChord, 4, 0, 0.12f, { 0, 0, 0.1f, -0.1f });
    return s;
}

Snapshot pTranceRoll()
{
    auto s = blank (7, 2, true);
    setTrack (s, 0, { 16, 4, 0, 36, 120, D1_16 });
    setTrack (s, 1, { 16, 4, 2, 46, 90, D1_16, 100, 0, 25 });
    setTrack (s, 2, { 16, 16, 0, 55, 85, D1_16, 100, 0, 35, 0, 1 });
    setTrack (s, 3, { 16, 13, 0, 43, 100, D1_16, 100, 0, 40, 0, 2 });
    lane (s, 2, tPitch, 16, 0, 0.3f, { 0, 0.34f, 0.67f, 1, 0, 0.34f, 0.67f, 0.34f, 0, 0.34f, 0.67f, 1, 0.67f, 0.34f, 0, -0.34f });
    sineLane (s, 2, tVelocity, 64, 0.25f, 2);
    return s;
}

Snapshot pHipHopBoom()
{
    auto s = blank (0, 13, true);
    setTrack (s, 0, { 16, 3, 0, 36, 120, D1_16, 100, 25 });
    setTrack (s, 1, { 16, 2, 4, 38, 110, D1_16, 100, 25 });
    setTrack (s, 2, { 16, 8, 0, 42, 75, D1_16, 90, 25, 20 });
    setTrack (s, 3, { 12, 5, 0, 60, 90, D1_16, 60, 25, 70, 1, 1 });
    lane (s, 2, tVelocity, 4, 0, 0.3f, { 0.4f, -0.4f, 0.1f, -0.6f });
    return s;
}

Snapshot pTrapHats()
{
    auto s = blank (4, 2, true);
    setTrack (s, 0, { 16, 3, 0, 36, 120, D1_16 });
    setTrack (s, 1, { 16, 2, 4, 39, 110, D1_16 });
    setTrack (s, 2, { 16, 12, 0, 42, 85, D1_16, 100, 0, 15 });
    setTrack (s, 3, { 8, 3, 1, 42, 70, D1_32t, 50, 0, 15 });
    setTrack (s, 4, { 16, 3, 0, 40, 110, D1_16, 100, 0, 90, 0, 2 });
    lane (s, 2, tClock, 8, 3, 0.12f, { 0, 0, 0, 1, 0, 0, 0.5f, 0 });
    return s;
}

Snapshot pLofiDust()
{
    auto s = blank (3, 1, true);
    setTrack (s, 0, { 16, 3, 0, 36, 100, D1_16, 100, 33 });
    setTrack (s, 1, { 16, 2, 4, 38, 90, D1_16, 100, 33 });
    setTrack (s, 2, { 16, 7, 0, 42, 60, D1_16, 80, 33, 20 });
    setTrack (s, 3, { 8, 2, 0, 63, 75, D1_8, 100, 33, 190, 2, 1 });
    setTrack (s, 4, { 11, 4, 0, 75, 65, D1_8, 55, 33, 60, 0, 2 });
    lane (s, 3, tPitch, 4, 0, 0.17f, { 0, -0.75f, -0.5f, 0.25f });
    lane (s, 4, tPitch, 16, 3, 0.25f, { 0, 0.5f, 1, 0.25f, -0.5f, 0.75f });
    return s;
}

Snapshot pSynthwaveDrive()
{
    auto s = blank (9, 2, true);
    setTrack (s, 0, { 16, 4, 0, 36, 115, D1_16 });
    setTrack (s, 1, { 16, 2, 4, 38, 110, D1_16 });
    setTrack (s, 2, { 16, 8, 0, 42, 80, D1_16, 100, 0, 30 });
    setTrack (s, 3, { 16, 16, 0, 33, 95, D1_16, 100, 0, 45, 7, 1 });
    setTrack (s, 4, { 16, 1, 0, 57, 80, D1_1, 100, 0, 100, 2, 2 });
    lane (s, 3, tPitch, 64, 0, 0.21f, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, -0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,-0.6f,
                                       -0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f,-0.4f, -0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f,-0.2f });
    return s;
}

Snapshot pAmbientDrift()
{
    auto s = blank (4, 7, true);
    setTrack (s, 0, { 7, 2, 0, 64, 70, D1_8, 80, 0, 180, 1, 1 });
    setTrack (s, 1, { 11, 3, 0, 71, 60, D1_8, 70, 0, 150, 0, 2 });
    setTrack (s, 2, { 13, 4, 2, 76, 55, D1_8t, 60, 0, 120, 0, 3 });
    setTrack (s, 3, { 5, 1, 0, 40, 75, D1_2, 100, 0, 200, 7, 4 });
    sineLane (s, 0, tPitch, 28, 0.25f);
    lane (s, 1, tPitch, 9, 3, 0.3f, { 0, 0.3f, 0.6f, -0.3f, 0.9f, 0.15f });
    sineLane (s, 2, tVelocity, 40, 0.3f, 1);
    return s;
}

Snapshot pChillPolyrhythm()
{
    auto s = blank (2, 5, true);
    setTrack (s, 0, { 12, 3, 0, 36, 90, D1_8t });
    setTrack (s, 1, { 16, 4, 2, 37, 70, D1_16, 100, 20 });
    setTrack (s, 2, { 5, 2, 0, 62, 70, D1_8, 85, 20, 90, 0, 1 });
    setTrack (s, 3, { 7, 3, 0, 69, 65, D1_8, 85, 20, 90, 0, 1 });
    setTrack (s, 4, { 9, 4, 0, 74, 60, D1_8, 70, 20, 60, 0, 2 });
    lane (s, 4, tPitch, 6, 2, 0.2f, { 0, 0.4f, 0.8f, 0.2f });
    return s;
}

Snapshot pPopPulse()
{
    auto s = blank (0, 1, true);
    setTrack (s, 0, { 16, 4, 0, 36, 115, D1_16 });
    setTrack (s, 1, { 16, 2, 4, 39, 105, D1_16 });
    setTrack (s, 2, { 16, 8, 1, 42, 75, D1_16, 100, 10, 20 });
    setTrack (s, 3, { 8, 3, 0, 60, 90, D1_8, 100, 10, 70, 1, 1 });
    setTrack (s, 4, { 16, 7, 0, 36, 100, D1_16, 100, 10, 45, 0, 2 });
    lane (s, 3, tPitch, 32, 0, 0.21f, { 0,0,0,0,0,0,0,0, 0.67f,0.67f,0.67f,0.67f,0.67f,0.67f,0.67f,0.67f, 0.86f,0.86f,0.86f,0.86f,0.86f,0.86f,0.86f,0.86f, 0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f });
    lane (s, 4, tPitch, 32, 0, 0.21f, { 0,0,0,0,0,0,0,0, 0.67f,0.67f,0.67f,0.67f,0.67f,0.67f,0.67f,0.67f, 0.86f,0.86f,0.86f,0.86f,0.86f,0.86f,0.86f,0.86f, 0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f });
    return s;
}

Snapshot pIndieShuffle()
{
    auto s = blank (7, 8, true);
    setTrack (s, 0, { 12, 4, 0, 36, 110, D1_8t });
    setTrack (s, 1, { 12, 2, 3, 38, 100, D1_8t });
    setTrack (s, 2, { 12, 9, 0, 51, 70, D1_8t, 90, 0, 30 });
    setTrack (s, 3, { 10, 3, 0, 67, 85, D1_8t, 100, 0, 80, 1, 1 });
    lane (s, 3, tChord, 3, 0, 0.06f, { 0, 1, -1 });
    return s;
}

Snapshot pProgressiveArp()
{
    auto s = blank (11, 2, true);
    setTrack (s, 0, { 16, 4, 0, 36, 118, D1_16 });
    setTrack (s, 1, { 16, 4, 2, 46, 85, D1_16, 100, 0, 25 });
    setTrack (s, 2, { 16, 11, 0, 59, 85, D1_16, 100, 0, 40, 0, 1 });
    setTrack (s, 3, { 15, 6, 0, 71, 70, D1_16, 90, 0, 35, 0, 2 });
    setTrack (s, 4, { 32, 2, 0, 47, 80, D1_16, 100, 0, 200, 2, 3 });
    sineLane (s, 2, tPitch, 48, 0.2f, 1);
    lane (s, 3, tPitch, 10, 2, 0.25f, { 0, 0.25f, 0.5f, 0.75f, 1 });
    lane (s, 2, tBeats, 64, 0, 0.08f, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,
                                       -0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 });
    return s;
}

Snapshot pProgressiveShift()
{
    auto s = blank (1, 5, true);
    setTrack (s, 0, { 16, 4, 0, 36, 118, D1_16 });
    setTrack (s, 1, { 16, 5, 0, 42, 80, D1_16, 100, 6, 20 });
    setTrack (s, 2, { 5, 3, 0, 61, 85, D1_16, 100, 0, 50, 0, 1 });
    setTrack (s, 3, { 6, 4, 1, 49, 95, D1_16, 100, 0, 50, 0, 2 });
    setTrack (s, 4, { 64, 1, 0, 61, 60, D1_16, 100, 0, 200, 3, 3 });
    lane (s, 2, tRotate, 16, 0, 0.05f, { 0, 0, 0, 0, 0.33f, 0.33f, 0.33f, 0.33f, 0.67f, 0.67f, 0.67f, 0.67f, 1, 1, 1, 1 });
    lane (s, 3, tChance, 8, 3, 0.5f, { 0, -0.4f, 0, -0.8f, 0, 0, -0.2f, 0 });
    return s;
}

const Def defs[] = {
    { "Techno", "Drive Tool", pTechnoDrive },
    { "Techno", "Hypnose", pTechnoHypnotic },
    { "House", "House Groove", pHouseGroove },
    { "Trance", "Trance Roll", pTranceRoll },
    { "Hip Hop", "Boom Bap Kreis", pHipHopBoom },
    { "Trap", "Trap Hats", pTrapHats },
    { "Lofi", "Staub", pLofiDust },
    { "Synthwave", "Nachtfahrt", pSynthwaveDrive },
    { "Ambient", "Drift", pAmbientDrift },
    { "Chill & Downtempo", "Polyrhythmus", pChillPolyrhythm },
    { "Pop", "Pop Puls", pPopPulse },
    { "Indie & Rock", "Indie Shuffle", pIndieShuffle },
    { "Progressive", "Arp Wandel", pProgressiveArp },
    { "Progressive", "Verschiebung", pProgressiveShift },
};
// ------------------------------------------------------------------ text-defined library
// Each library preset is a compact spec, segments separated by ';':
//   header : <root> <scale> [q]                       e.g. "A moll q"
//   track  : T <steps> <beats> <rot> <pitch> <vel> <div> [c=chance s=swing g=gate h=chord m=channel l=level]
//            pitch = MIDI number, note name (C1 = 36) or drum alias (kick, clap, chh ...).
//            Drum aliases/numbers default to channel 10, note names to channel 1.
//   lane   : L <track> <target> <len> <f|r|p|z> <values in target units ...>   ("5x4" repeats 5 four times)
//   sine   : S <track> <target> <len> <amplitude in target units> [cycles]
struct LibDef { const char* cat; const char* name; const char* spec; };

const LibDef library[] = {
#include "PresetLibrary.inc"
};

int indexOf (const juce::String& tok, std::initializer_list<const char*> list)
{
    int i = 0;
    for (auto* n : list) { if (tok == n) return i; ++i; }
    return tok.isNotEmpty() && tok.containsOnly ("0123456789") ? tok.getIntValue() : -1;
}

int noteFromName (const juce::String& tok)
{
    static const std::pair<const char*, int> drums[] = {
        { "kick", 36 }, { "kick2", 35 }, { "rim", 37 }, { "snare", 38 }, { "clap", 39 }, { "snare2", 40 },
        { "chh", 42 }, { "phh", 44 }, { "ohh", 46 }, { "crash", 49 }, { "ride", 51 }, { "bell", 53 },
        { "tamb", 54 }, { "cow", 56 }, { "ftom", 41 }, { "ltom", 45 }, { "mtom", 47 }, { "htom", 50 },
        { "bongo", 60 }, { "bongolo", 61 }, { "congam", 62 }, { "conga", 63 }, { "congalo", 64 },
        { "timb", 65 }, { "timblo", 66 }, { "agogo", 67 }, { "cabasa", 69 }, { "maracas", 70 },
        { "guiro", 73 }, { "clave", 75 }, { "wood", 76 }, { "woodlo", 77 }, { "tri", 81 }, { "shaker", 82 },
        { "splash", 55 }, { "china", 52 }, { "vibra", 58 } };
    for (auto& d : drums) if (tok == d.first) return d.second;
    if (tok.isNotEmpty() && tok.containsOnly ("0123456789")) return tok.getIntValue();
    static const char* const names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int pc = 11; pc >= 0; --pc)
    {
        const int n = (int) std::strlen (names[pc]);
        const auto rest = tok.substring (n);
        if (tok.startsWith (names[pc]) && rest.isNotEmpty() && rest.containsOnly ("-0123456789"))
            return (rest.getIntValue() + 2) * 12 + pc;
    }
    return -1;
}

int scaleIndex (const juce::String& t)
{
    return indexOf (t, { "chr", "dur", "moll", "hmoll", "mmoll", "dor", "phr", "lyd", "mix", "lok", "phrd", "ung",
                         "pdur", "pmoll", "blues", "ganz" });
}

int chordIndex (const juce::String& t)
{
    return indexOf (t, { "aus", "tri", "sept", "none", "ssus2", "ssus4", "quart", "5", "8", "dur", "moll", "maj7",
                         "m7", "7", "dim", "aug", "sus2", "sus4", "add9" });
}

int divisionIndex (const juce::String& t)
{
    for (int i = 0; i < kNumDivisions; ++i) if (t == divisions[i].name) return i;
    return -1;
}

int targetIndex (const juce::String& t)
{
    return indexOf (t, { "steps", "beats", "rot", "chance", "vel", "swing", "pitch", "chord", "clock" });
}

bool parseSpec (const char* spec, Snapshot& s, juce::String& err)
{
    s = blank (0, 0, false);
    for (int i = 0; i < kTracks; ++i)
        for (int g = 0; g < kTargets; ++g)
            s.lanes[(size_t) i][(size_t) g].length = 16;
    const auto segs = juce::StringArray::fromTokens (juce::String::fromUTF8 (spec), ";", "");
    int track = 0;
    bool header = false;
    for (auto seg : segs)
    {
        auto tk = juce::StringArray::fromTokens (seg.trim(), " ", "");
        tk.removeEmptyStrings();
        if (tk.isEmpty()) continue;
        if (! header)
        {
            header = true;
            s.root = indexOf (tk[0], { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" });
            s.scale = tk.size() > 1 ? scaleIndex (tk[1]) : 0;
            s.quantize = tk.contains ("q");
            if (s.root < 0 || s.root > 11 || s.scale < 0 || s.scale >= kNumScales) { err = "header: " + seg; return false; }
            continue;
        }
        if (tk[0] == "T")
        {
            if (tk.size() < 7 || track >= kTracks) { err = "track: " + seg; return false; }
            auto& t = s.tracks[(size_t) track];
            t = TrackSettings();
            t.steps = tk[1].getIntValue(); t.beats = tk[2].getIntValue(); t.rotate = tk[3].getIntValue();
            t.pitch = noteFromName (tk[4]);
            t.channel = juce::CharacterFunctions::isUpperCase (tk[4][0]) ? 1 : 10;
            t.velocity = tk[5].getIntValue();
            t.clock = divisionIndex (tk[6]);
            for (int i = 7; i < tk.size(); ++i)
            {
                const auto key = tk[i].upToFirstOccurrenceOf ("=", false, false);
                const auto val = tk[i].fromFirstOccurrenceOf ("=", false, false);
                if (key == "c") t.chance = val.getFloatValue();
                else if (key == "s") t.swing = val.getFloatValue();
                else if (key == "g") t.gate = val.getFloatValue();
                else if (key == "h") t.chord = chordIndex (val);
                else if (key == "m") t.channel = val.getIntValue();
                else if (key == "l") t.level = val.getFloatValue();
                else { err = "key " + tk[i] + " in: " + seg; return false; }
            }
            if (t.steps < 1 || t.steps > kMaxSteps || t.beats < 0 || t.beats > t.steps || t.rotate < 0 || t.rotate >= kMaxSteps
                || t.pitch < 0 || t.pitch > 127 || t.velocity < 1 || t.velocity > 127 || t.clock < 0
                || t.chord < 0 || t.chord >= kNumChords || t.channel < 1 || t.channel > 16
                || t.chance < 0 || t.chance > 100 || t.level < 0 || t.level > 100 || t.swing < 0 || t.swing > 75 || t.gate < 5 || t.gate > 200)
            { err = "range: " + seg; return false; }
            ++track;
            continue;
        }
        if (tk[0] == "L" || tk[0] == "S")
        {
            const int tr = tk[1].getIntValue() - 1;
            const int tg = targetIndex (tk[2]);
            const int len = tk[3].getIntValue();
            if (tk.size() < 5 || tr < 0 || tr >= track || tg < 0 || tg >= kTargets || len < 2 || len > kMaxSteps)
            { err = "lane: " + seg; return false; }
            auto& l = s.lanes[(size_t) tr][(size_t) tg];
            l.on = true; l.length = len; l.depth = 1.0f; l.v = {};
            const float range = (float) targetRange[tg];
            if (tk[0] == "S")
            {
                const float amp = tk[4].getFloatValue() / range;
                const float cyc = tk.size() > 5 ? tk[5].getFloatValue() : 1.0f;
                for (int i = 0; i < len; ++i)
                    l.v[(size_t) i] = juce::jlimit (-1.0f, 1.0f, amp * std::sin (juce::MathConstants<float>::twoPi * cyc * (float) i / (float) len));
                continue;
            }
            l.dir = indexOf (tk[4], { "f", "r", "p", "z" });
            if (l.dir < 0 || l.dir > 3) { err = "dir: " + seg; return false; }
            std::vector<float> vals;
            for (int i = 5; i < tk.size(); ++i)
            {
                int rep = 1;
                auto v = tk[i];
                if (v.containsChar ('x'))
                {
                    rep = v.fromFirstOccurrenceOf ("x", false, false).getIntValue();
                    v = v.upToFirstOccurrenceOf ("x", false, false);
                }
                if (rep < 1 || v.isEmpty() || ! v.containsOnly ("-.0123456789")) { err = "value " + tk[i] + " in: " + seg; return false; }
                for (int r = 0; r < rep; ++r) vals.push_back (v.getFloatValue() / range);
            }
            if (vals.empty() || (int) vals.size() > len) { err = "lane values: " + seg; return false; }
            for (int i = 0; i < len; ++i) l.v[(size_t) i] = juce::jlimit (-1.0f, 1.0f, vals[(size_t) i % vals.size()]);
            continue;
        }
        err = "unknown: " + seg;
        return false;
    }
    if (track == 0) { err = "no tracks"; return false; }
    return true;
}

constexpr int kCodePresets = (int) (sizeof (defs) / sizeof (defs[0]));
constexpr int kLibPresets = (int) (sizeof (library) / sizeof (library[0]));
} // namespace

int numFactoryPresets() { return kCodePresets + kLibPresets; }

juce::String factoryCategory (int i)
{
    i = juce::jlimit (0, numFactoryPresets() - 1, i);
    return juce::String::fromUTF8 (i < kCodePresets ? defs[i].cat : library[i - kCodePresets].cat);
}

juce::String factoryName (int i)
{
    i = juce::jlimit (0, numFactoryPresets() - 1, i);
    return juce::String::fromUTF8 (i < kCodePresets ? defs[i].name : library[i - kCodePresets].name);
}

Snapshot makeFactoryPreset (int i)
{
    i = juce::jlimit (0, numFactoryPresets() - 1, i);
    if (i < kCodePresets) return defs[i].make();
    Snapshot s;
    juce::String err;
    if (! parseSpec (library[i - kCodePresets].spec, s, err)) { jassertfalse; return defaultSnapshot(); }
    return s;
}

juce::String validateFactoryPresets()
{
    juce::String errors;
    for (int i = 0; i < kLibPresets; ++i)
    {
        Snapshot s;
        juce::String err;
        if (! parseSpec (library[i].spec, s, err))
            errors << juce::String::fromUTF8 (library[i].name) << ": " << err << "\n";
    }
    juce::StringArray names;
    for (int i = 0; i < numFactoryPresets(); ++i)
    {
        if (names.contains (factoryName (i))) errors << "doppelter Name " << factoryName (i) << "\n";
        names.add (factoryName (i));
    }
    return errors;
}

// Fixed genre order for the menu (categories not listed are appended).
juce::StringArray factoryCategories()
{
    const juce::StringArray order { "Techno", "Minimal & Tech House", "House", "Trance", "Progressive", "Psytrance",
                                    "Drum & Bass", "Breakbeat & Garage", "Dubstep & Bass", "Hard Dance",
                                    "Dark Electro & EBM", "Industrial", "Synthwave", "Hip Hop", "Trap", "Lofi",
                                    "Chill & Downtempo", "Ambient", "Pop", "Indie & Rock", "Funk & Disco",
                                    "Afro & Latin", "Reggae & Dub", "Jazz & Soul", "IDM & Experimental", "Cinematic", "Demos" };
    juce::StringArray used;
    for (int i = 0; i < numFactoryPresets(); ++i) used.addIfNotAlreadyThere (factoryCategory (i));
    juce::StringArray out;
    for (auto& c : order) if (used.contains (c)) out.add (c);
    for (auto& c : used) out.addIfNotAlreadyThere (c);
    return out;
}

Snapshot defaultSnapshot()
{
    auto s = blank (0, 0, false);
    setTrack (s, 0, { 16, 4, 0, 36, 110, D1_16 });
    setTrack (s, 1, { 16, 2, 4, 38, 100, D1_16 });
    setTrack (s, 2, { 16, 8, 0, 42, 80, D1_16, 100, 0, 25 });
    setTrack (s, 3, { 12, 5, 0, 46, 85, D1_16, 100, 0, 25 });
    const int pitches[] = { 0, 0, 0, 0, 39, 45, 49, 51 };
    for (int i = 4; i < kTracks; ++i)
    {
        auto& t = s.tracks[(size_t) i];
        t = TrackSettings();
        t.active = false; t.pitch = pitches[i]; t.channel = 10; t.steps = 16; t.beats = 3;
    }
    for (int i = 0; i < kTracks; ++i)
        for (int g = 0; g < kTargets; ++g)
            s.lanes[(size_t) i][(size_t) g].length = 16;
    return s;
}
} // namespace kr
