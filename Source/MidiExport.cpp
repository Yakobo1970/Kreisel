#include "MidiExport.h"

namespace kr
{
juce::MidiFile renderMidi (const Snapshot& s, int bars, double bpm, juce::uint32 seed)
{
    const double totalQ = 4.0 * juce::jmax (1, bars);
    std::array<juce::MidiMessageSequence, kTracks> seqs;

    Sequencer seq;
    seq.reset();
    juce::Random rng ((juce::int64) seed);
    auto toTicks = [] (double q) { return std::round (q * kExportPpq); };

    seq.process (s, 0.0, totalQ, rng,
                 [] (int, const ResolvedStep&, double) {},
                 [&] (const NoteEvent& e) {
                     const double on = toTicks (e.q);
                     const double off = juce::jmin (toTicks (totalQ), juce::jmax (on + 1.0, toTicks (e.q + e.lenQ)));
                     for (int i = 0; i < e.numNotes; ++i)
                     {
                         seqs[(size_t) e.track].addEvent (juce::MidiMessage::noteOn (e.channel, e.notes[i], (juce::uint8) e.velocity), on);
                         seqs[(size_t) e.track].addEvent (juce::MidiMessage::noteOff (e.channel, e.notes[i]), off);
                     }
                 });

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (kExportPpq);

    juce::MidiMessageSequence meta;
    meta.addEvent (juce::MidiMessage::textMetaEvent (3, "Kreisel"), 0);
    meta.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / juce::jlimit (20.0, 999.0, bpm))), 0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    meta.addEvent (juce::MidiMessage::endOfTrack(), toTicks (totalQ));
    mf.addTrack (meta);

    for (int t = 0; t < kTracks; ++t)
    {
        if (! s.tracks[(size_t) t].active) continue;
        auto& sq = seqs[(size_t) t];
        sq.addEvent (juce::MidiMessage::textMetaEvent (3, "Kreisel Spur " + juce::String (t + 1)), 0);
        sq.updateMatchedPairs();
        sq.sort();
        sq.addEvent (juce::MidiMessage::endOfTrack(), toTicks (totalQ));
        mf.addTrack (sq);
    }
    return mf;
}

bool writeMidiFile (const juce::MidiFile& mf, const juce::File& file)
{
    file.deleteFile();
    juce::FileOutputStream os (file);
    if (! os.openedOk()) return false;
    return mf.writeTo (os, 1);
}
} // namespace kr
