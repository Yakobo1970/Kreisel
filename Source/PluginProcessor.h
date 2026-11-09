#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Sequencer.h"
#include "MidiExport.h"
#include "SoundEngine.h"
#include "HostSettings.h"

namespace kr
{
juce::String pid (int track, const char* name); // "t3_steps"

// Randomiser targets: the nine lane targets plus gate.
constexpr int kRandTargets = kTargets + 1;
inline const char* const randNames[kRandTargets] = { "Steps", "Beats", "Rotate", "Chance", "Velocity",
                                                     "Swing", "Pitch", "Chord", "Clock", "Gate" };
} // namespace kr

class KreiselProcessor : public juce::AudioProcessor, private juce::ValueTree::Listener
{
public:
    KreiselProcessor();
    ~KreiselProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Kreisel"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---------------------------------------------------------------- model access (message thread)
    juce::UndoManager undo { 1, 40 }; // keeps exactly the last 40 transactions
    juce::AudioProcessorValueTreeState apvts;

    juce::RangedAudioParameter* param (const juce::String& id) const { return apvts.getParameter (id); }
    void setParam (const juce::String& id, float plainValue);
    float getParam (const juce::String& id) const;

    juce::ValueTree laneTree (int track, int target);
    juce::ValueTree uiTree();
    juce::ValueTree randTree();
    kr::LaneSettings getLane (int track, int target) const;
    void setLaneValues (int track, int target, const std::array<float, kr::kMaxSteps>& v);

    kr::Snapshot makeSnapshot() const;
    void applySnapshot (const kr::Snapshot& s, const juce::String& presetName);   // undoable
    static juce::XmlElement snapshotToXml (const kr::Snapshot& s);
    static bool snapshotFromXml (const juce::XmlElement& x, kr::Snapshot& s);

    bool saveUserPreset (const juce::File& f);
    bool loadUserPreset (const juce::File& f);
    static juce::File userPresetFolder();
    void loadFactoryPreset (int index);
    juce::String currentPresetName() { return uiTree().getProperty ("preset", "Init").toString(); }

    // track names: custom (UI tree "name1".."name8") or derived from sound / GM note / channel
    juce::String trackName (int track);
    juce::String customTrackName (int track);
    void setTrackName (int track, const juce::String& name); // undoable, empty = automatic

    // ---------------------------------------------------------------- hosted plug-ins (message thread)
    // Every track can carry a VST3 instrument (replaces the built-in sound) and a VST3 effect.
    enum SlotKind { slotInstrument = 0, slotEffect = 1 };
    juce::String loadHosted (int track, int kind, const juce::PluginDescription& desc, const juce::MemoryBlock* state = nullptr);
    void unloadHosted (int track, int kind);
    bool hasHosted (int track, int kind) const;
    juce::String hostedName (int track, int kind) const;
    juce::String hostedId (int track, int kind) const;
    void showHostedEditor (int track, int kind);
    juce::SharedResourcePointer<kr::HostSettings> hostSettings;

    void randomise (int track /* -1 = all */);
    void randomiseLane (int track, int target);

    juce::File exportMidiToTemp (int bars);
    juce::MidiFile renderMidi (int bars) const { return kr::renderMidi (makeSnapshot(), bars, exportBpm()); }
    double exportBpm() const;

    // ---------------------------------------------------------------- visual feedback (audio -> UI)
    struct Vis
    {
        std::atomic<int> pos { -1 }, steps { 16 }, beats { 4 }, rotate { 0 };
        std::atomic<juce::int64> k { 0 };
        std::atomic<juce::uint32> hits { 0 }, stepsFired { 0 };
        std::atomic<int> lastVelocity { 0 };
    };
    std::array<Vis, kr::kTracks> vis;
    std::atomic<bool> transportRunning { false }, hostTransport { false };
    std::atomic<double> currentPpq { 0.0 }, currentBpm { 120.0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void ensureTrees();
    void syncLane (const juce::ValueTree& lane);
    void syncAllLanes();
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeRedirected (juce::ValueTree&) override { syncAllLanes(); }
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override { syncAllLanes(); }

    struct AtomicLane
    {
        std::atomic<bool> on { false };
        std::atomic<int> length { 16 }, dir { 0 };
        std::atomic<float> depth { 1.0f };
        std::array<std::atomic<float>, kr::kMaxSteps> v {};
    };
    std::array<std::array<AtomicLane, kr::kTargets>, kr::kTracks> lanes;

    struct TrackParams
    {
        std::atomic<float>* p[14] {};
    };
    std::array<TrackParams, kr::kTracks> tp;
    std::atomic<float>* pRoot = nullptr, * pScale = nullptr, * pQuant = nullptr, * pRun = nullptr, * pBpm = nullptr;
    std::atomic<float>* pIntern = nullptr, * pVol = nullptr;
    std::array<std::atomic<float>*, kr::kTracks> pSnd {}, pLvl {};

    // audio-thread state
    struct PendingOff { double q; int ch, note, track; };
    struct OutEvent { int sample; bool off; int ch, note, vel, track; };
    kr::SoundEngine engine;

    struct Slot
    {
        std::unique_ptr<juce::AudioPluginInstance> inst;
        juce::PluginDescription desc;
        juce::AudioBuffer<float> buf;
    };
    std::array<std::array<Slot, 2>, kr::kTracks> slots;
    juce::CriticalSection hostLock;
    std::array<std::unique_ptr<juce::DocumentWindow>, kr::kTracks * 2> hostedWindows;
    std::array<juce::MidiBuffer, kr::kTracks> trackMidi;
    juce::MidiBuffer emptyMidi;
    juce::AudioBuffer<float> trackBuf { 2, 4096 };
    int maxBlock = 512;
    struct InnerPlayHead : juce::AudioPlayHead
    {
        PositionInfo info;
        juce::Optional<PositionInfo> getPosition() const override { return info; }
    } innerPlayHead;
    void mixTracks (juce::AudioBuffer<float>& out, int numSamples, bool intern, float master);
    juce::ValueTree hostedStateTree();
    void restoreHosted (const juce::ValueTree& hosted);
    void closeHostedEditor (int track, int kind);
    static void configureLayout (juce::AudioPluginInstance& p, bool effect);
    kr::Sequencer seq;
    kr::Snapshot audioSnap;
    std::vector<PendingOff> pendingOffs;
    std::vector<OutEvent> outEvents;
    juce::Random rng;
    double sr = 44100.0;
    double internalPpq = 0.0, lastToQ = -1.0e12;
    bool wasRunning = false, internalWasOn = false;
    int currentProgram = 0;

    void fillSnapshot (kr::Snapshot& s) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KreiselProcessor)
};
