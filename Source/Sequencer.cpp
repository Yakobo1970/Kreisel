#include "Sequencer.h"

namespace kr
{
static inline juce::int64 posMod (juce::int64 a, juce::int64 m) { return ((a % m) + m) % m; }

static inline juce::uint32 hash3 (juce::int64 k, int a, int b)
{
    auto x = (juce::uint64) k * 0x9E3779B97F4A7C15ull ^ (juce::uint64) (a * 131 + b * 7919 + 17);
    x ^= x >> 33; x *= 0xff51afd7ed558ccdull; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ull; x ^= x >> 33;
    return (juce::uint32) x;
}

int laneIndex (const LaneSettings& lane, juce::int64 k, int track, int target)
{
    const int len = juce::jlimit (2, kMaxSteps, lane.length);
    int idx = 0;
    switch (lane.dir)
    {
        case 1: idx = len - 1 - (int) posMod (k, len); break;
        case 2:
        {
            const int period = 2 * len - 2;
            const int p = (int) posMod (k, period);
            idx = p < len ? p : period - p;
            break;
        }
        case 3: idx = (int) (hash3 (k, track, target) % (juce::uint32) len); break;
        default: idx = (int) posMod (k, len); break;
    }
    return idx;
}

float laneValue (const LaneSettings& lane, juce::int64 k, int track, int target)
{
    if (! lane.on) return 0.0f;
    const int idx = laneIndex (lane, k, track, target);
    return juce::jlimit (-1.0f, 1.0f, lane.v[(size_t) idx]) * juce::jlimit (0.0f, 1.0f, lane.depth);
}

ResolvedStep resolveStep (const Snapshot& s, int t, juce::int64 k)
{
    const auto& ts = s.tracks[(size_t) t];
    const auto& lanes = s.lanes[(size_t) t];
    auto mod = [&] (int target) { return laneValue (lanes[(size_t) target], k, t, target) * (float) targetRange[target]; };

    ResolvedStep r;
    r.steps = juce::jlimit (1, kMaxSteps, ts.steps + juce::roundToInt (mod (tSteps)));
    r.beats = juce::jlimit (0, r.steps, ts.beats + juce::roundToInt (mod (tBeats)));
    r.rotate = juce::jlimit (0, kMaxSteps - 1, ts.rotate + juce::roundToInt (mod (tRotate)));
    r.chance = juce::jlimit (0.0f, 100.0f, ts.chance + mod (tChance));
    r.velocity = juce::jlimit (1, 127, ts.velocity + juce::roundToInt (mod (tVelocity)));
    r.swing = juce::jlimit (0.0f, 75.0f, ts.swing + mod (tSwing));
    r.pitch = ts.pitch;
    r.chord = juce::jlimit (0, kNumChords - 1, ts.chord + juce::roundToInt (mod (tChord)));
    r.division = juce::jlimit (0, kNumDivisions - 1, ts.clock + juce::roundToInt (mod (tClock)));

    const int pitchShift = juce::roundToInt (mod (tPitch));
    if (s.quantize && ts.channel != 10) // drum kits (GM channel 10) are never quantised
    {
        // pitch lane moves in scale degrees
        const auto& sc = scales[juce::jlimit (0, kNumScales - 1, s.scale)];
        int note = snapToScale (ts.pitch, s.root, s.scale);
        int rel = note - s.root;
        int oct = (int) std::floor (rel / 12.0);
        int within = rel - oct * 12;
        int idx = 0;
        for (int i = 0; i < sc.count; ++i) if (sc.steps[i] == within) idx = i;
        int deg = oct * sc.count + idx + pitchShift;
        int o2 = (int) std::floor ((double) deg / sc.count);
        int i2 = deg - o2 * sc.count;
        r.pitch = s.root + o2 * 12 + sc.steps[i2];
    }
    else
    {
        r.pitch = ts.pitch + pitchShift;
    }
    r.pitch = juce::jlimit (0, 127, r.pitch);

    r.k = k;
    r.pos = (int) posMod (k, r.steps);
    r.hit = patternHit (r.pos, r.steps, r.beats, r.rotate);
    return r;
}

int snapToScale (int note, int root, int scaleIdx)
{
    const auto& sc = scales[juce::jlimit (0, kNumScales - 1, scaleIdx)];
    int best = note, bestDist = 99;
    for (int o = -1; o <= 1; ++o)
        for (int i = 0; i < sc.count; ++i)
        {
            const int base = note - ((note - root) % 12 + 12) % 12; // root below note
            const int cand = base + o * 12 + sc.steps[i];
            const int d = std::abs (cand - note);
            if (d < bestDist || (d == bestDist && cand < best)) { best = cand; bestDist = d; }
        }
    return best;
}

int buildNotes (const Snapshot& s, const ResolvedStep& r, int* out)
{
    const auto& ch = chords[juce::jlimit (0, kNumChords - 1, r.chord)];
    int n = 0;
    if (! ch.scale)
    {
        for (int i = 0; i < ch.count; ++i)
            out[n++] = juce::jlimit (0, 127, r.pitch + ch.iv[i]);
        return n;
    }
    // diatonic: stack scale degrees (chromatic scale falls back to major)
    const int scaleIdx = s.scale == 0 ? 1 : juce::jlimit (0, kNumScales - 1, s.scale);
    const auto& sc = scales[scaleIdx];
    const int base = snapToScale (r.pitch, s.root, scaleIdx);
    const int rel = base - s.root;
    const int oct = (int) std::floor (rel / 12.0);
    const int within = rel - oct * 12;
    int idx = 0;
    for (int i = 0; i < sc.count; ++i) if (sc.steps[i] == within) idx = i;
    for (int i = 0; i < ch.count; ++i)
    {
        const int deg = idx + ch.iv[i];
        const int o2 = deg / sc.count;
        out[n++] = juce::jlimit (0, 127, s.root + (oct + o2) * 12 + sc.steps[deg % sc.count]);
    }
    return n;
}
} // namespace kr
