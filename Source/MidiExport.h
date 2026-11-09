#pragma once
#include "Sequencer.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace kr
{
constexpr int kExportPpq = 960;

// Renders `bars` bars of 4/4 into a type-1 MIDI file (one MIDI track per active sequencer track).
juce::MidiFile renderMidi (const Snapshot& s, int bars, double bpm, juce::uint32 seed = 0x4b52);
bool writeMidiFile (const juce::MidiFile& mf, const juce::File& file);
} // namespace kr
