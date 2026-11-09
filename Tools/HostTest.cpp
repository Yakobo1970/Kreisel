// Loads the built Kreisel.vst3 like a DAW: host transport, MIDI output, note-off balance, loop, stop, state round-trip.
// Usage: HostTest <path-to.vst3>
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <map>

struct FakePlayHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 124;
    bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setIsPlaying (playing);
        p.setBpm (bpm);
        p.setPpqPosition (ppq);
        return p;
    }
};

static int failures = 0;
#define CHECK(cond, msg) do { if (! (cond)) { ++failures; std::cout << "FAIL: " << msg << "\n"; } else std::cout << "ok:   " << msg << "\n"; } while (0)

struct Stats { int ons = 0, offs = 0, maxHanging = 0; std::map<int, int> hanging; };

static void run (juce::AudioPluginInstance& inst, FakePlayHead& ph, double sr, double fromQ, double toQ, Stats& st, int block = 512)
{
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    const double qPerSample = ph.bpm / 60.0 / sr;
    for (double q = fromQ; q < toQ; q += block * qPerSample)
    {
        ph.ppq = q;
        midi.clear();
        inst.processBlock (buf, midi);
        for (const auto m : midi)
        {
            const auto msg = m.getMessage();
            const int key = msg.getChannel() * 128 + msg.getNoteNumber();
            if (msg.isNoteOn()) { ++st.ons; ++st.hanging[key]; }
            else if (msg.isNoteOff()) { ++st.offs; --st.hanging[key]; }
        }
    }
}

static int hangingNotes (const Stats& st)
{
    int n = 0;
    for (auto& [k, v] : st.hanging) n += v;
    return n;
}

int main (int argc, char** argv)
{
    if (argc < 2) { std::cout << "usage: HostTest <Kreisel.vst3>\n"; return 2; }
    juce::ScopedJuceInitialiser_GUI gui;
    juce::AudioPluginFormatManager fm;
    fm.addFormat (std::make_unique<juce::VST3PluginFormat>());
    juce::OwnedArray<juce::PluginDescription> descs;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (descs, juce::String (argv[1]));
    CHECK (descs.size() == 1, "found plugin description");
    if (descs.isEmpty()) return 1;
    std::cout << "      " << descs[0]->name << " by " << descs[0]->manufacturerName << ", instrument=" << (int) descs[0]->isInstrument << "\n";

    const double sr = 48000.0;
    juce::String err;
    auto inst = fm.createPluginInstance (*descs[0], sr, 512, err);
    CHECK (inst != nullptr, "instance created " << err);
    if (! inst) return 1;
    CHECK (inst->producesMidi(), "plugin produces MIDI");
    CHECK (inst->getNumPrograms() >= 14, "factory programs exposed (" << inst->getNumPrograms() << ")");
    CHECK (inst->getParameters().size() >= 8 * 14, "parameters exposed (" << inst->getParameters().size() << ")");

    FakePlayHead ph;
    inst->setPlayHead (&ph);
    inst->prepareToPlay (sr, 512);

    // default pattern: 4 bars
    Stats st;
    run (*inst, ph, sr, 0.0, 16.0, st);
    std::cout << "      default 4 bars: " << st.ons << " note-ons\n";
    CHECK (st.ons > 50, "default pattern plays notes");
    CHECK (hangingNotes (st) <= 8, "only the last notes are still held (" << hangingNotes (st) << ")");

    // stop -> all notes off
    ph.playing = false;
    run (*inst, ph, sr, 16.0, 16.5, st);
    CHECK (hangingNotes (st) == 0, "stop releases every note");

    // loop jump back -> no hanging notes after release
    ph.playing = true;
    Stats lp;
    run (*inst, ph, sr, 0.0, 7.9, lp);
    run (*inst, ph, sr, 0.0, 4.0, lp);  // relocate
    ph.playing = false;
    run (*inst, ph, sr, 4.0, 4.2, lp);
    CHECK (hangingNotes (lp) == 0, "loop relocation keeps note-ons/offs balanced");

    // block size independence: 64 vs 2048 sample blocks give same note count
    Stats a, b;
    ph.playing = true;
    inst->prepareToPlay (sr, 2048);
    run (*inst, ph, sr, 0.0, 32.0, a, 64);
    ph.playing = false; run (*inst, ph, sr, 32.0, 32.2, a, 64); ph.playing = true;
    run (*inst, ph, sr, 0.0, 32.0, b, 2048);
    ph.playing = false; run (*inst, ph, sr, 32.0, 32.2, b, 2048);
    CHECK (a.ons == b.ons, "block size independent (" << a.ons << " vs " << b.ons << ")");

    // built-in sounds: audible by default, silent when switched off
    {
        auto* intern = (juce::AudioProcessorParameter*) nullptr;
        for (auto* p : inst->getParameters())
            if (p->getName (64) == juce::String::fromUTF8 ("Interne Klänge")) intern = p;
        CHECK (intern != nullptr, "parameter Interne Klaenge exposed");
        auto peakOf = [&] {
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer m;
            float peak = 0.0f;
            ph.playing = true;
            for (double q = 0.0; q < 4.0; q += 512 * (ph.bpm / 60.0 / sr))
            {
                ph.ppq = q;
                inst->processBlock (buf, m);
                peak = juce::jmax (peak, buf.getMagnitude (0, 512));
            }
            ph.playing = false;
            ph.ppq = 4.0;
            for (int i = 0; i < 300; ++i) inst->processBlock (buf, m); // tails + note-offs
            return peak;
        };
        inst->prepareToPlay (sr, 512);
        const float on = peakOf();
        if (intern) intern->setValueNotifyingHost (0.0f);
        const float off = peakOf();
        if (intern) intern->setValueNotifyingHost (1.0f);
        CHECK (on > 0.05f && on <= 1.0f, "built-in sounds play (peak " << on << ")");
        CHECK (off == 0.0f, "switching built-in sounds off gives silence (peak " << off << ")");
    }

    // program change + state round-trip
    inst->setCurrentProgram (3); // Trance Roll: no chance < 100 %, deterministic

    juce::MemoryBlock state;
    inst->getStateInformation (state);
    auto inst2 = fm.createPluginInstance (*descs[0], sr, 512, err);
    inst2->setStateInformation (state.getData(), (int) state.getSize());
    juce::MemoryBlock state2;
    inst2->getStateInformation (state2);
    CHECK (state.getSize() > 1000 && state == state2, "state round-trip identical (" << (int) state.getSize() << " bytes)");

    Stats p1, p2;
    FakePlayHead ph2;
    ph.playing = true;
    inst->setPlayHead (&ph); inst2->setPlayHead (&ph2);
    inst->prepareToPlay (sr, 512); inst2->prepareToPlay (sr, 512);
    run (*inst, ph, sr, 0.0, 16.0, p1);
    run (*inst2, ph2, sr, 0.0, 16.0, p2);
    CHECK (p1.ons == p2.ons && p1.ons > 0, "restored instance plays the same (" << p1.ons << " vs " << p2.ons << ")");

    // editor opens
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (inst->createEditorIfNeeded());
        CHECK (ed != nullptr && ed->getWidth() == 1200, "editor opens at 1200x800");
    }
    inst->releaseResources();
    inst2->releaseResources();
    std::cout << (failures == 0 ? "\nALL PASSED\n" : "\nFAILURES\n");
    return failures == 0 ? 0 : 1;
}
