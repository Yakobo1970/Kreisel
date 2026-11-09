// Kreisel engine regression test. Usage: EngineTest [out.mid]
#include "../Source/Sequencer.h"
#include "../Source/Presets.h"
#include "../Source/MidiExport.h"
#include "../Source/SoundEngine.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <tuple>
#include <cmath>

using namespace kr;

static int failures = 0;
#define CHECK(cond, msg) do { if (! (cond)) { ++failures; std::cout << "FAIL: " << msg << "\n"; } else std::cout << "ok:   " << msg << "\n"; } while (0)

struct Ev { int track; double q; int note; int vel; };

static std::vector<Ev> render (const Snapshot& s, double total, double block)
{
    Sequencer seq;
    seq.reset();
    juce::Random rng (1234);
    std::vector<Ev> out;
    for (double q = 0; q < total - 1e-9; q += block)
        seq.process (s, q, juce::jmin (total, q + block), rng, [] (int, const ResolvedStep&, double) {},
                     [&] (const NoteEvent& e) { for (int i = 0; i < e.numNotes; ++i) out.push_back ({ e.track, e.q, e.notes[i], e.velocity }); });
    return out;
}

int main (int argc, char** argv)
{
    // 1) euclid
    juce::String pat;
    for (int i = 0; i < 8; ++i) pat << (patternHit (i, 8, 3, 0) ? "x" : ".");
    CHECK (pat == "x..x..x.", "E(3,8) = " << pat);
    pat.clear();
    for (int i = 0; i < 8; ++i) pat << (patternHit (i, 8, 3, 1) ? "x" : ".");
    CHECK (pat == ".x..x..x", "E(3,8) rot 1 = " << pat);
    int cnt = 0;
    for (int n = 1; n <= 64; ++n)
        for (int b = 0; b <= n; ++b)
        {
            int h = 0;
            for (int i = 0; i < n; ++i) h += patternHit (i, n, b, 0) ? 1 : 0;
            cnt += (h == b) ? 0 : 1;
        }
    CHECK (cnt == 0, "beat count matches for all 1..64 step patterns");

    // 2) four-on-the-floor at 1/16
    Snapshot s = defaultSnapshot();
    for (int t = 1; t < kTracks; ++t) s.tracks[(size_t) t].active = false;
    auto ev = render (s, 16.0, 0.37);
    CHECK (ev.size() == 16, "16/4 over 4 bars gives 16 notes (got " << (int) ev.size() << ")");
    bool onBeats = true;
    for (auto& e : ev) onBeats &= std::abs (e.q - std::round (e.q)) < 1e-9;
    CHECK (onBeats, "hits land on quarter notes");

    // 3) block-size independence
    s = defaultSnapshot();
    s.tracks[2].swing = 30;
    s.tracks[3].clock = 14; // 1/16T
    auto a = render (s, 32.0, 32.0), b = render (s, 32.0, 0.0123);
    bool same = a.size() == b.size();
    for (size_t i = 0; same && i < a.size(); ++i) same = a[i].track == b[i].track && std::abs (a[i].q - b[i].q) < 1e-6 && a[i].note == b[i].note;
    // order within one block can differ (per track), compare as multiset
    if (! same && a.size() == b.size())
    {
        auto key = [] (const Ev& e) { return std::make_tuple (e.q, e.track, e.note); };
        std::sort (a.begin(), a.end(), [&] (auto& x, auto& y) { return key (x) < key (y); });
        std::sort (b.begin(), b.end(), [&] (auto& x, auto& y) { return key (x) < key (y); });
        same = true;
        for (size_t i = 0; same && i < a.size(); ++i) same = a[i].track == b[i].track && std::abs (a[i].q - b[i].q) < 1e-6;
    }
    CHECK (same, "one big block == many small blocks (" << (int) a.size() << " vs " << (int) b.size() << ")");

    // 4) swing offsets odd steps
    s = defaultSnapshot();
    for (int t = 0; t < kTracks; ++t) s.tracks[(size_t) t].active = t == 0;
    s.tracks[0].beats = 16; s.tracks[0].swing = 50;
    ev = render (s, 1.0, 0.1);
    CHECK (ev.size() == 4 && std::abs (ev[1].q - (0.25 + 0.125)) < 1e-9, "swing 50 % shifts step 2 by half a step");

    // 5) chords
    s = defaultSnapshot();
    s.root = 0; s.scale = 1;
    ResolvedStep r; r.pitch = 60; r.chord = 1;
    int notes[kMaxChordNotes];
    int n = buildNotes (s, r, notes);
    CHECK (n == 3 && notes[0] == 60 && notes[1] == 64 && notes[2] == 67, "C-Dur Skala-Dreiklang auf C");
    r.pitch = 62;
    n = buildNotes (s, r, notes);
    CHECK (n == 3 && notes[0] == 62 && notes[1] == 65 && notes[2] == 69, "C-Dur Skala-Dreiklang auf D = d-Moll");
    s.root = 9; s.scale = 2; r.pitch = 57; r.chord = 2;
    n = buildNotes (s, r, notes);
    CHECK (n == 4 && notes[0] == 57 && notes[1] == 60 && notes[2] == 64 && notes[3] == 67, "a-Moll Skala-Septakkord = Am7");
    CHECK (snapToScale (61, 0, 1) == 60, "C# snaps into C major as C");

    // 6) modulation lanes: pitch lane in scale degrees, clock lane does not hang
    s = defaultSnapshot();
    for (int t = 0; t < kTracks; ++t) s.tracks[(size_t) t].active = t == 0;
    s.quantize = true; s.root = 0; s.scale = 1;
    s.tracks[0].pitch = 60; s.tracks[0].beats = 16; s.tracks[0].channel = 1;
    auto& pl = s.lanes[0][tPitch];
    pl.on = true; pl.length = 4; pl.depth = 1.0f;
    pl.v = {}; pl.v[1] = 1.0f / 24.0f; pl.v[2] = 2.0f / 24.0f; pl.v[3] = -1.0f / 24.0f;
    ev = render (s, 1.0, 0.25);
    CHECK (ev.size() == 4 && ev[0].note == 60 && ev[1].note == 62 && ev[2].note == 64 && ev[3].note == 59,
           "pitch lane walks scale degrees (60 62 64 59)");
    auto& cl = s.lanes[0][tClock];
    cl.on = true; cl.length = 3; cl.depth = 1.0f; cl.v = {}; cl.v[1] = 3.0f / 17.0f; cl.v[2] = -3.0f / 17.0f;
    ev = render (s, 16.0, 0.05);
    CHECK (ev.size() > 10 && ev.size() < 200, "clock lane renders (" << (int) ev.size() << " notes in 4 bars)");

    // 6b) drum tracks (channel 10) ignore scale quantisation
    {
        auto d = defaultSnapshot();
        for (int t = 0; t < kTracks; ++t) d.tracks[(size_t) t].active = t == 0;
        d.quantize = true; d.root = 2; d.scale = 6; // D phrygian has no F#
        d.tracks[0].pitch = 42; d.tracks[0].channel = 10; d.tracks[0].beats = 4;
        auto de = render (d, 4.0, 0.25);
        bool allHat = ! de.empty();
        for (auto& x : de) allHat &= x.note == 42;
        CHECK (allHat, "channel 10 hi-hat stays on note 42 with quantise on");
    }

    // 7) all factory presets
    {
        const auto errs = validateFactoryPresets();
        if (errs.isNotEmpty()) std::cout << errs;
        CHECK (errs.isEmpty(), "preset library parses, names unique (" << numFactoryPresets() << " presets, "
               << factoryCategories().size() << " genres)");
    }
    bool allOk = true;
    for (int i = 0; i < numFactoryPresets(); ++i)
    {
        auto p = makeFactoryPreset (i);
        auto e = render (p, 64.0, 0.2);
        bool ok = ! e.empty();
        for (auto& x : e) ok &= x.note >= 0 && x.note <= 127 && x.vel >= 1 && x.vel <= 127;
        if (! ok) std::cout << "  preset " << factoryName (i) << " failed\n";
        allOk &= ok;
        std::cout << "  " << factoryCategory (i) << " / " << factoryName (i) << ": " << (int) e.size() << " notes in 16 bars\n";
    }
    CHECK (allOk, "all " << numFactoryPresets() << " factory presets render valid notes");

    // 7b) demos keep evolving: over 64 bars no 4-bar section is an exact copy of another
    {
        bool allOk = true;
        for (int i = 0; i < numFactoryPresets(); ++i)
        {
            if (factoryCategory (i) != "Demos") continue;
            auto p = makeFactoryPreset (i);
            int active = 0;
            for (auto& t : p.tracks) active += t.active ? 1 : 0;
            auto e = render (p, 256.0, 0.25);
            std::vector<juce::String> sections (16);
            for (auto& x : e)
            {
                const int sec = juce::jlimit (0, 15, (int) (x.q / 16.0));
                sections[(size_t) sec] << x.track << ":" << juce::String (std::fmod (x.q, 16.0), 3) << ":" << x.note << ":" << x.vel << " ";
            }
            juce::StringArray distinct;
            for (auto& sct : sections) distinct.addIfNotAlreadyThere (sct);
            std::cout << "  demo " << factoryName (i) << ": " << active << " tracks, " << distinct.size() << "/16 different 4-bar sections\n";
            allOk &= active == 8 && distinct.size() == 16;
        }
        CHECK (allOk, "every demo uses all 8 tracks and never repeats a 4-bar section within 64 bars");
    }

    // 8) MIDI export
    auto mf = renderMidi (makeFactoryPreset (0), 4, 128.0);
    CHECK (mf.getTimeFormat() == 960, "export uses 960 PPQ");
    int active = 0;
    for (auto& t : makeFactoryPreset (0).tracks) active += t.active ? 1 : 0;
    CHECK (mf.getNumTracks() == active + 1, "one MIDI track per active track + tempo track");
    double last = 0;
    int ons = 0;
    for (int t = 1; t < mf.getNumTracks(); ++t)
        for (auto* m : *mf.getTrack (t))
        {
            last = juce::jmax (last, m->message.getTimeStamp());
            ons += m->message.isNoteOn() ? 1 : 0;
        }
    CHECK (last <= 4 * 4 * 960 && ons > 0, "export stays within 4 bars (" << ons << " notes)");
    if (argc > 1)
        CHECK (writeMidiFile (mf, juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])), "wrote " << argv[1]);

    // 9) built-in sounds: every sound produces a finite, audible, non-clipping signal and ends
    {
        const double sr = 48000.0;
        bool allOk = true;
        for (int snd = sKit; snd < kNumSounds; ++snd)
        {
            std::vector<int> notes;
            if (snd == sKit) for (int n = 35; n <= 82; ++n) notes.push_back (n);
            else notes = { 36, 48, 60, 72 };
            for (int note : notes)
            {
                SoundEngine eng;
                eng.prepare (sr);
                eng.noteOn (0, snd, note, 110, 0.8f);
                std::vector<float> l (512), r (512);
                float peak = 0.0f;
                bool finite = true;
                int blocks = 0;
                for (; blocks < (int) (6.0 * sr / 512); ++blocks)
                {
                    if (blocks == (int) (0.4 * sr / 512)) eng.noteOff (0, note);
                    std::fill (l.begin(), l.end(), 0.0f);
                    std::fill (r.begin(), r.end(), 0.0f);
                    eng.render (l.data(), r.data(), 512);
                    for (int i = 0; i < 512; ++i)
                    {
                        finite &= std::isfinite (l[(size_t) i]) && std::isfinite (r[(size_t) i]);
                        peak = juce::jmax (peak, std::abs (l[(size_t) i]), std::abs (r[(size_t) i]));
                    }
                    if (eng.activeVoices() == 0) break;
                }
                const bool ok = finite && peak > 0.02f && peak <= 1.0f && eng.activeVoices() == 0;
                if (! ok)
                    std::cout << "  sound " << juce::String::fromUTF8 (soundNames[snd]) << " note " << note << ": peak " << peak
                              << " finite " << (int) finite << " voices left " << eng.activeVoices() << "\n";
                allOk &= ok;
            }
        }
        CHECK (allOk, "all built-in sounds are audible, finite, below 0 dBFS and release");

        SoundEngine eng; // voice stealing under load
        eng.prepare (sr);
        std::vector<float> l (256), r (256);
        bool finite = true;
        for (int b = 0; b < 400; ++b)
        {
            for (int k = 0; k < 6; ++k) eng.noteOn (k, k < 3 ? sKit : sPad, 36 + (b * 7 + k * 5) % 50, 100, 1.0f);
            std::fill (l.begin(), l.end(), 0.0f);
            std::fill (r.begin(), r.end(), 0.0f);
            eng.render (l.data(), r.data(), 256);
            for (int i = 0; i < 256; ++i) finite &= std::isfinite (l[(size_t) i]) && std::abs (l[(size_t) i]) <= 8.0f;
        }
        CHECK (finite, "voice stealing under heavy load stays finite and bounded");
    }

    // 10) track names
    {
        TrackSettings t;
        t.channel = 10; t.pitch = 42;
        CHECK (autoTrackName (t) == "Hi-Hat", "channel 10 note 42 is named Hi-Hat");
        t.channel = 2; t.pitch = 33;
        CHECK (autoTrackName (t) == "Bass" && resolveSound (t) == sSub, "channel 2 is Bass with a sub sound for low notes");
        t.name = "Mein Bass";
        CHECK (autoTrackName (t) == "Mein Bass", "custom name wins");
        t.name = {}; t.sound = sBell;
        CHECK (autoTrackName (t) == "Glocke" && resolveSound (t) == sBell, "explicit sound names the track");
    }

    std::cout << (failures == 0 ? "\nALL PASSED\n" : "\nFAILURES: ") << (failures == 0 ? "" : juce::String (failures).toRawUTF8()) << "\n";
    return failures == 0 ? 0 : 1;
}
