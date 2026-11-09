// Kreisel – sequencing core: eight independent euclidean tracks, each with its own clock,
// length, swing and nine modulation lanes. Shared by the plugin and the MIDI export.
#pragma once
#include "Model.h"
#include <juce_core/juce_core.h>
#include <cmath>

namespace kr
{
struct NoteEvent
{
    int track = 0;
    double q = 0;      // position in quarter notes
    double lenQ = 0;   // length in quarter notes
    int channel = 1, velocity = 100;
    int numNotes = 0;
    int notes[kMaxChordNotes] {};
};

// One evaluated step of a track (after modulation).
struct ResolvedStep
{
    int steps = 16, beats = 4, rotate = 0;
    float chance = 100, swing = 0;
    int velocity = 100, pitch = 36, chord = 0, division = kDefaultDivision;
    int pos = 0;
    juce::int64 k = 0;   // absolute step counter (drives the lanes)
    bool hit = false;
};

int laneIndex (const LaneSettings& lane, juce::int64 k, int track, int target);
float laneValue (const LaneSettings& lane, juce::int64 k, int track, int target);
ResolvedStep resolveStep (const Snapshot& s, int track, juce::int64 k);
int buildNotes (const Snapshot& s, const ResolvedStep& r, int* out);   // returns number of notes
int snapToScale (int note, int root, int scale);

class Sequencer
{
public:
    void reset()
    {
        lastTo = -1.0e12;
        for (auto& r : run) r.valid = false;
    }

    // Generates all steps in [fromQ, toQ). onStep(track, ResolvedStep, q) is called for every step
    // (hit or not), onNote(NoteEvent) for every audible note.
    template <typename StepFn, typename NoteFn>
    void process (const Snapshot& s, double fromQ, double toQ, juce::Random& rng, StepFn&& onStep, NoteFn&& onNote)
    {
        if (toQ <= fromQ) return;
        const bool jumped = std::abs (fromQ - lastTo) > 1.0e-4;
        lastTo = toQ;

        bool anySolo = false;
        for (auto& t : s.tracks) anySolo |= (t.active && t.solo);

        for (int t = 0; t < kTracks; ++t)
        {
            const auto& ts = s.tracks[(size_t) t];
            auto& r = run[(size_t) t];
            if (! ts.active) { r.valid = false; continue; }

            const bool clockModulated = s.lanes[(size_t) t][tClock].on;
            const int baseDiv = juce::jlimit (0, kNumDivisions - 1, ts.clock);
            if (! r.valid || jumped || (! clockModulated && r.div != baseDiv))
            {
                const double len = divisions[baseDiv].quarters;
                r.k = (juce::int64) std::ceil (fromQ / len - 1.0e-9);
                r.gridQ = (double) r.k * len;
                r.valid = true;
            }
            r.div = clockModulated ? -2 : baseDiv;

            for (int guard = 0; guard < 8192; ++guard)
            {
                const auto st = resolveStep (s, t, r.k);
                const double len = divisions[st.division].quarters;
                const double fire = r.gridQ + ((r.k & 1) != 0 ? len * st.swing / 100.0 : 0.0);
                if (fire >= toQ) break;
                if (fire >= fromQ - 1.0e-9)
                {
                    onStep (t, st, fire);
                    const bool audible = st.hit && ! ts.mute && (! anySolo || ts.solo)
                                         && (st.chance >= 100.0f || rng.nextFloat() * 100.0f < st.chance);
                    if (audible)
                    {
                        NoteEvent e;
                        e.track = t;
                        e.q = fire;
                        e.lenQ = juce::jmax (1.0 / 960.0, len * ts.gate / 100.0);
                        e.channel = juce::jlimit (1, 16, ts.channel);
                        e.velocity = juce::jlimit (1, 127, st.velocity);
                        e.numNotes = buildNotes (s, st, e.notes);
                        onNote (e);
                    }
                }
                r.gridQ += len;
                ++r.k;
            }
        }
    }

private:
    struct Run { bool valid = false; juce::int64 k = 0; double gridQ = 0; int div = -1; };
    std::array<Run, kTracks> run {};
    double lastTo = -1.0e12;
};
} // namespace kr
