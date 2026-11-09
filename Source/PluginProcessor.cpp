#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace kr;

namespace
{
const char* const trackParamNames[14] = { "active", "mute", "solo", "steps", "beats", "rotate", "chance",
                                          "vel", "swing", "pitch", "chord", "clock", "gate", "chan" };
enum { pActive, pMute, pSolo, pSteps, pBeats, pRotate, pChance, pVel, pSwing, pPitch, pChord, pClock, pGate, pChan };

juce::StringArray divisionNames()
{
    juce::StringArray a;
    for (auto& d : divisions) a.add (d.name);
    return a;
}
juce::StringArray chordNames()
{
    juce::StringArray a;
    for (auto& c : chords) a.add (juce::String::fromUTF8 (c.name));
    return a;
}
juce::StringArray soundNameList()
{
    juce::StringArray a;
    for (auto* n : soundNames) a.add (juce::String::fromUTF8 (n));
    return a;
}
juce::StringArray scaleNames()
{
    juce::StringArray a;
    for (auto& s : scales) a.add (juce::String::fromUTF8 (s.name));
    return a;
}

juce::String valuesToString (const std::array<float, kMaxSteps>& v)
{
    juce::String s;
    for (int i = 0; i < kMaxSteps; ++i)
        s << (i ? " " : "") << juce::String (v[(size_t) i], 3);
    return s;
}

std::array<float, kMaxSteps> valuesFromString (const juce::String& str)
{
    std::array<float, kMaxSteps> v {};
    juce::StringArray tok;
    tok.addTokens (str, " ", "");
    for (int i = 0; i < juce::jmin (kMaxSteps, tok.size()); ++i)
        v[(size_t) i] = juce::jlimit (-1.0f, 1.0f, tok[i].getFloatValue());
    return v;
}

const juce::Identifier idLanes ("LANES"), idLane ("LANE"), idUi ("UI"), idRand ("RAND");
const juce::Identifier idT ("t"), idG ("g"), idOn ("on"), idLen ("len"), idDir ("dir"), idDepth ("depth"), idVals ("vals");

// default randomiser ranges: min, max, enabled
struct RandDef { float lo, hi; bool on; };
const RandDef randDefaults[kRandTargets] = {
    { 4, 32, true }, { 1, 12, true }, { 0, 15, true }, { 60, 100, false }, { 60, 127, true },
    { 0, 30, false }, { 36, 72, false }, { 0, 0, false }, { 10, 14, false }, { 15, 100, false }
};
} // namespace

juce::String kr::pid (int track, const char* name) { return "t" + juce::String (track + 1) + "_" + name; }

// ===================================================================================== layout
juce::AudioProcessorValueTreeState::ParameterLayout KreiselProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    const auto def = defaultSnapshot();

    for (int t = 0; t < kTracks; ++t)
    {
        const auto& d = def.tracks[(size_t) t];
        const String pre = "Spur " + String (t + 1) + " ";
        auto group = std::make_unique<AudioProcessorParameterGroup> ("track" + String (t + 1), "Spur " + String (t + 1), " | ");
        auto intP = [&] (const char* n, const String& label, int lo, int hi, int dv) {
            group->addChild (std::make_unique<AudioParameterInt> (ParameterID { pid (t, n), 1 }, pre + label, lo, hi, dv));
        };
        auto floatP = [&] (const char* n, const String& label, float lo, float hi, float dv, const String& suffix) {
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { pid (t, n), 1 }, pre + label,
                NormalisableRange<float> (lo, hi, 0.1f), dv, AudioParameterFloatAttributes().withLabel (suffix)));
        };
        group->addChild (std::make_unique<AudioParameterBool> (ParameterID { pid (t, "active"), 1 }, pre + "Aktiv", d.active));
        group->addChild (std::make_unique<AudioParameterBool> (ParameterID { pid (t, "mute"), 1 }, pre + "Stumm", false));
        group->addChild (std::make_unique<AudioParameterBool> (ParameterID { pid (t, "solo"), 1 }, pre + "Solo", false));
        intP ("steps", "Steps", 1, kMaxSteps, d.steps);
        intP ("beats", "Beats", 0, kMaxSteps, d.beats);
        intP ("rotate", "Rotate", 0, kMaxSteps - 1, d.rotate);
        floatP ("chance", "Chance", 0.0f, 100.0f, d.chance, "%");
        intP ("vel", "Velocity", 1, 127, d.velocity);
        floatP ("swing", "Swing", 0.0f, 75.0f, d.swing, "%");
        group->addChild (std::make_unique<AudioParameterInt> (ParameterID { pid (t, "pitch"), 1 }, pre + "Pitch", 0, 127, d.pitch,
            AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return midiNoteName (v); })));
        group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { pid (t, "chord"), 1 }, pre + "Chord", chordNames(), d.chord));
        group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { pid (t, "clock"), 1 }, pre + "Clock", divisionNames(), d.clock));
        floatP ("gate", "Gate", 5.0f, 200.0f, d.gate, "%");
        intP ("chan", "MIDI-Kanal", 1, 16, d.channel);
        group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { pid (t, "snd"), 1 }, pre + "Klang", soundNameList(), d.sound));
        floatP ("lvl", "Pegel", 0.0f, 100.0f, d.level, "%");
        layout.add (std::move (group));
    }

    StringArray roots;
    for (auto* n : noteNames) roots.add (n);
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "root", 1 }, "Grundton", roots, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Skala", scaleNames(), 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "quant", 1 }, "Skala quantisieren", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "run", 1 }, "Interner Lauf", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "Internes Tempo",
        NormalisableRange<float> (40.0f, 240.0f, 0.1f), 120.0f, AudioParameterFloatAttributes().withLabel ("BPM")));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "intern", 1 }, juce::String::fromUTF8 ("Interne Klänge"), true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "vol", 1 }, juce::String::fromUTF8 ("Lautstärke"),
        NormalisableRange<float> (-30.0f, 6.0f, 0.1f), -6.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    return layout;
}

// ===================================================================================== ctor
KreiselProcessor::KreiselProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "KREISEL", createLayout())
{
    for (int t = 0; t < kTracks; ++t)
        for (int i = 0; i < 14; ++i)
            tp[(size_t) t].p[i] = apvts.getRawParameterValue (pid (t, trackParamNames[i]));
    pRoot = apvts.getRawParameterValue ("root");
    pScale = apvts.getRawParameterValue ("scale");
    pQuant = apvts.getRawParameterValue ("quant");
    pRun = apvts.getRawParameterValue ("run");
    pBpm = apvts.getRawParameterValue ("bpm");
    pIntern = apvts.getRawParameterValue ("intern");
    pVol = apvts.getRawParameterValue ("vol");
    for (int t = 0; t < kTracks; ++t)
    {
        pSnd[(size_t) t] = apvts.getRawParameterValue (pid (t, "snd"));
        pLvl[(size_t) t] = apvts.getRawParameterValue (pid (t, "lvl"));
    }

    ensureTrees();
    apvts.state.addListener (this);
    syncAllLanes();
    if (wrapperType == wrapperType_Standalone) // no host transport: run on the internal clock right away
        if (auto* p = apvts.getParameter ("run")) p->setValueNotifyingHost (1.0f);
    pendingOffs.reserve (2048);
    outEvents.reserve (4096);
}

KreiselProcessor::~KreiselProcessor()
{
    apvts.state.removeListener (this);
    for (auto& w : hostedWindows) w.reset();
    const juce::ScopedLock sl (hostLock);
    for (auto& tr : slots)
        for (auto& s : tr) s.inst.reset();
}

void KreiselProcessor::ensureTrees()
{
    auto lanesT = apvts.state.getOrCreateChildWithName (idLanes, nullptr);
    const auto def = defaultSnapshot();
    for (int t = 0; t < kTracks; ++t)
        for (int g = 0; g < kTargets; ++g)
        {
            bool found = false;
            for (auto c : lanesT)
                if ((int) c[idT] == t && (int) c[idG] == g) { found = true; break; }
            if (found) continue;
            const auto& l = def.lanes[(size_t) t][(size_t) g];
            juce::ValueTree n (idLane);
            n.setProperty (idT, t, nullptr).setProperty (idG, g, nullptr).setProperty (idOn, l.on, nullptr)
             .setProperty (idLen, l.length, nullptr).setProperty (idDir, l.dir, nullptr)
             .setProperty (idDepth, l.depth, nullptr).setProperty (idVals, valuesToString (l.v), nullptr);
            lanesT.appendChild (n, nullptr);
        }
    auto ui = apvts.state.getOrCreateChildWithName (idUi, nullptr);
    if (! ui.hasProperty ("preset")) ui.setProperty ("preset", "Init", nullptr);
    auto rnd = apvts.state.getOrCreateChildWithName (idRand, nullptr);
    for (int i = 0; i < kRandTargets; ++i)
    {
        const juce::String n (randNames[i]);
        if (! rnd.hasProperty (n + "_min")) rnd.setProperty (n + "_min", randDefaults[i].lo, nullptr);
        if (! rnd.hasProperty (n + "_max")) rnd.setProperty (n + "_max", randDefaults[i].hi, nullptr);
        if (! rnd.hasProperty (n + "_on")) rnd.setProperty (n + "_on", randDefaults[i].on, nullptr);
    }
}

juce::ValueTree KreiselProcessor::laneTree (int track, int target)
{
    for (auto c : apvts.state.getChildWithName (idLanes))
        if ((int) c[idT] == track && (int) c[idG] == target) return c;
    jassertfalse;
    return {};
}

juce::ValueTree KreiselProcessor::uiTree() { return apvts.state.getOrCreateChildWithName (idUi, nullptr); }
juce::ValueTree KreiselProcessor::randTree() { return apvts.state.getOrCreateChildWithName (idRand, nullptr); }

void KreiselProcessor::syncLane (const juce::ValueTree& c)
{
    const int t = c[idT], g = c[idG];
    if (! juce::isPositiveAndBelow (t, kTracks) || ! juce::isPositiveAndBelow (g, kTargets)) return;
    auto& l = lanes[(size_t) t][(size_t) g];
    l.on = (bool) c[idOn];
    l.length = juce::jlimit (2, kMaxSteps, (int) c.getProperty (idLen, 16));
    l.dir = juce::jlimit (0, kNumLaneDirs - 1, (int) c[idDir]);
    l.depth = juce::jlimit (0.0f, 1.0f, (float) c.getProperty (idDepth, 1.0f));
    const auto v = valuesFromString (c[idVals].toString());
    for (int i = 0; i < kMaxSteps; ++i) l.v[(size_t) i] = v[(size_t) i];
}

void KreiselProcessor::syncAllLanes()
{
    for (auto c : apvts.state.getChildWithName (idLanes))
        syncLane (c);
}

void KreiselProcessor::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier&)
{
    if (tree.hasType (idLane)) syncLane (tree);
}

kr::LaneSettings KreiselProcessor::getLane (int t, int g) const
{
    const auto& a = lanes[(size_t) t][(size_t) g];
    LaneSettings l;
    l.on = a.on; l.length = a.length; l.dir = a.dir; l.depth = a.depth;
    for (int i = 0; i < kMaxSteps; ++i) l.v[(size_t) i] = a.v[(size_t) i];
    return l;
}

void KreiselProcessor::setLaneValues (int track, int target, const std::array<float, kMaxSteps>& v)
{
    laneTree (track, target).setProperty (idVals, valuesToString (v), &undo);
}

void KreiselProcessor::setParam (const juce::String& id, float plain)
{
    if (auto* p = param (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

float KreiselProcessor::getParam (const juce::String& id) const
{
    if (auto* v = apvts.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

// ===================================================================================== snapshot
void KreiselProcessor::fillSnapshot (Snapshot& s) const
{
    for (int t = 0; t < kTracks; ++t)
    {
        const auto& p = tp[(size_t) t].p;
        auto& tr = s.tracks[(size_t) t];
        tr.active = p[pActive]->load() > 0.5f;
        tr.mute = p[pMute]->load() > 0.5f;
        tr.solo = p[pSolo]->load() > 0.5f;
        tr.steps = (int) p[pSteps]->load();
        tr.beats = (int) p[pBeats]->load();
        tr.rotate = (int) p[pRotate]->load();
        tr.chance = p[pChance]->load();
        tr.velocity = (int) p[pVel]->load();
        tr.swing = p[pSwing]->load();
        tr.pitch = (int) p[pPitch]->load();
        tr.chord = (int) p[pChord]->load();
        tr.clock = (int) p[pClock]->load();
        tr.gate = p[pGate]->load();
        tr.channel = (int) p[pChan]->load();
        tr.sound = (int) pSnd[(size_t) t]->load();
        tr.level = pLvl[(size_t) t]->load();
        for (int g = 0; g < kTargets; ++g)
        {
            const auto& a = lanes[(size_t) t][(size_t) g];
            auto& l = s.lanes[(size_t) t][(size_t) g];
            l.on = a.on; l.length = a.length; l.dir = a.dir; l.depth = a.depth;
            if (l.on)
                for (int i = 0; i < kMaxSteps; ++i) l.v[(size_t) i] = a.v[(size_t) i];
        }
    }
    s.root = (int) pRoot->load();
    s.scale = (int) pScale->load();
    s.quantize = pQuant->load() > 0.5f;
}

kr::Snapshot KreiselProcessor::makeSnapshot() const
{
    Snapshot s;
    fillSnapshot (s);
    auto ui = const_cast<KreiselProcessor*> (this)->uiTree();
    for (int t = 0; t < kTracks; ++t)
        s.tracks[(size_t) t].name = ui.getProperty ("name" + juce::String (t + 1)).toString();
    for (int t = 0; t < kTracks; ++t)  // full lane data (also for disabled lanes)
        for (int g = 0; g < kTargets; ++g)
            for (int i = 0; i < kMaxSteps; ++i)
                s.lanes[(size_t) t][(size_t) g].v[(size_t) i] = lanes[(size_t) t][(size_t) g].v[(size_t) i];
    return s;
}

void KreiselProcessor::applySnapshot (const Snapshot& s, const juce::String& presetName)
{
    undo.beginNewTransaction ("Preset " + presetName);
    for (int t = 0; t < kTracks; ++t)
    {
        const auto& tr = s.tracks[(size_t) t];
        setParam (pid (t, "active"), tr.active ? 1.0f : 0.0f);
        setParam (pid (t, "mute"), tr.mute ? 1.0f : 0.0f);
        setParam (pid (t, "solo"), tr.solo ? 1.0f : 0.0f);
        setParam (pid (t, "steps"), (float) tr.steps);
        setParam (pid (t, "beats"), (float) tr.beats);
        setParam (pid (t, "rotate"), (float) tr.rotate);
        setParam (pid (t, "chance"), tr.chance);
        setParam (pid (t, "vel"), (float) tr.velocity);
        setParam (pid (t, "swing"), tr.swing);
        setParam (pid (t, "pitch"), (float) tr.pitch);
        setParam (pid (t, "chord"), (float) tr.chord);
        setParam (pid (t, "clock"), (float) tr.clock);
        setParam (pid (t, "gate"), tr.gate);
        setParam (pid (t, "chan"), (float) tr.channel);
        setParam (pid (t, "snd"), (float) tr.sound);
        setParam (pid (t, "lvl"), tr.level);
        uiTree().setProperty ("name" + juce::String (t + 1), tr.name, &undo);
        for (int g = 0; g < kTargets; ++g)
        {
            const auto& l = s.lanes[(size_t) t][(size_t) g];
            auto lt = laneTree (t, g);
            lt.setProperty (idOn, l.on, &undo);
            lt.setProperty (idLen, l.length, &undo);
            lt.setProperty (idDir, l.dir, &undo);
            lt.setProperty (idDepth, l.depth, &undo);
            lt.setProperty (idVals, valuesToString (l.v), &undo);
        }
    }
    setParam ("root", (float) s.root);
    setParam ("scale", (float) s.scale);
    setParam ("quant", s.quantize ? 1.0f : 0.0f);
    uiTree().setProperty ("preset", presetName, &undo);
}

juce::XmlElement KreiselProcessor::snapshotToXml (const Snapshot& s)
{
    juce::XmlElement x ("KREISELPRESET");
    x.setAttribute ("version", 1);
    x.setAttribute ("root", s.root);
    x.setAttribute ("scale", s.scale);
    x.setAttribute ("quant", s.quantize);
    for (int t = 0; t < kTracks; ++t)
    {
        const auto& tr = s.tracks[(size_t) t];
        auto* e = x.createNewChildElement ("TRACK");
        e->setAttribute ("active", tr.active); e->setAttribute ("mute", tr.mute); e->setAttribute ("solo", tr.solo);
        e->setAttribute ("steps", tr.steps); e->setAttribute ("beats", tr.beats); e->setAttribute ("rotate", tr.rotate);
        e->setAttribute ("chance", tr.chance); e->setAttribute ("vel", tr.velocity); e->setAttribute ("swing", tr.swing);
        e->setAttribute ("pitch", tr.pitch); e->setAttribute ("chord", tr.chord); e->setAttribute ("clock", tr.clock);
        e->setAttribute ("gate", tr.gate); e->setAttribute ("chan", tr.channel);
        e->setAttribute ("sound", tr.sound); e->setAttribute ("level", tr.level);
        if (tr.name.isNotEmpty()) e->setAttribute ("name", tr.name);
        for (int g = 0; g < kTargets; ++g)
        {
            const auto& l = s.lanes[(size_t) t][(size_t) g];
            auto* le = e->createNewChildElement ("LANE");
            le->setAttribute ("target", targetNames[g]);
            le->setAttribute ("on", l.on); le->setAttribute ("len", l.length);
            le->setAttribute ("dir", l.dir); le->setAttribute ("depth", l.depth);
            le->setAttribute ("vals", valuesToString (l.v));
        }
    }
    return x;
}

bool KreiselProcessor::snapshotFromXml (const juce::XmlElement& x, Snapshot& s)
{
    if (! x.hasTagName ("KREISELPRESET")) return false;
    s = Snapshot();
    s.root = juce::jlimit (0, 11, x.getIntAttribute ("root"));
    s.scale = juce::jlimit (0, kNumScales - 1, x.getIntAttribute ("scale"));
    s.quantize = x.getBoolAttribute ("quant");
    int t = 0;
    for (auto* e : x.getChildWithTagNameIterator ("TRACK"))
    {
        if (t >= kTracks) break;
        auto& tr = s.tracks[(size_t) t];
        tr.active = e->getBoolAttribute ("active", true); tr.mute = e->getBoolAttribute ("mute"); tr.solo = e->getBoolAttribute ("solo");
        tr.steps = juce::jlimit (1, kMaxSteps, e->getIntAttribute ("steps", 16));
        tr.beats = juce::jlimit (0, kMaxSteps, e->getIntAttribute ("beats", 4));
        tr.rotate = juce::jlimit (0, kMaxSteps - 1, e->getIntAttribute ("rotate"));
        tr.chance = (float) e->getDoubleAttribute ("chance", 100);
        tr.velocity = juce::jlimit (1, 127, e->getIntAttribute ("vel", 100));
        tr.swing = (float) e->getDoubleAttribute ("swing");
        tr.pitch = juce::jlimit (0, 127, e->getIntAttribute ("pitch", 36));
        tr.chord = juce::jlimit (0, kNumChords - 1, e->getIntAttribute ("chord"));
        tr.clock = juce::jlimit (0, kNumDivisions - 1, e->getIntAttribute ("clock", kDefaultDivision));
        tr.gate = (float) e->getDoubleAttribute ("gate", 50);
        tr.channel = juce::jlimit (1, 16, e->getIntAttribute ("chan", 1));
        tr.sound = juce::jlimit (0, kNumSounds - 1, e->getIntAttribute ("sound", sAuto));
        tr.level = (float) juce::jlimit (0.0, 100.0, e->getDoubleAttribute ("level", 80.0));
        tr.name = e->getStringAttribute ("name");
        int g = 0;
        for (auto* le : e->getChildWithTagNameIterator ("LANE"))
        {
            if (g >= kTargets) break;
            auto& l = s.lanes[(size_t) t][(size_t) g];
            l.on = le->getBoolAttribute ("on"); l.length = juce::jlimit (2, kMaxSteps, le->getIntAttribute ("len", 16));
            l.dir = juce::jlimit (0, kNumLaneDirs - 1, le->getIntAttribute ("dir"));
            l.depth = (float) le->getDoubleAttribute ("depth", 1.0);
            l.v = valuesFromString (le->getStringAttribute ("vals"));
            ++g;
        }
        ++t;
    }
    return true;
}

juce::File KreiselProcessor::userPresetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Kreisel").getChildFile ("Presets");
    f.createDirectory();
    return f;
}

bool KreiselProcessor::saveUserPreset (const juce::File& f)
{
    auto x = snapshotToXml (makeSnapshot());
    x.setAttribute ("name", f.getFileNameWithoutExtension());
    if (! x.writeTo (f)) return false;
    uiTree().setProperty ("preset", f.getFileNameWithoutExtension(), nullptr);
    return true;
}

bool KreiselProcessor::loadUserPreset (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    Snapshot s;
    if (xml == nullptr || ! snapshotFromXml (*xml, s)) return false;
    applySnapshot (s, f.getFileNameWithoutExtension());
    return true;
}

juce::String KreiselProcessor::customTrackName (int track)
{
    return uiTree().getProperty ("name" + juce::String (track + 1)).toString();
}

juce::String KreiselProcessor::trackName (int track)
{
    Snapshot s;
    fillSnapshot (s);
    auto tr = s.tracks[(size_t) juce::jlimit (0, kTracks - 1, track)];
    tr.name = customTrackName (track);
    return autoTrackName (tr);
}

void KreiselProcessor::setTrackName (int track, const juce::String& name)
{
    undo.beginNewTransaction ("Spur umbenennen");
    uiTree().setProperty ("name" + juce::String (track + 1), name.trim(), &undo);
}

void KreiselProcessor::loadFactoryPreset (int index)
{
    applySnapshot (makeFactoryPreset (index), factoryName (index));
}

int KreiselProcessor::getNumPrograms() { return numFactoryPresets(); }
const juce::String KreiselProcessor::getProgramName (int i) { return factoryCategory (i) + " - " + factoryName (i); }
void KreiselProcessor::setCurrentProgram (int i)
{
    if (! juce::isPositiveAndBelow (i, numFactoryPresets())) return;
    currentProgram = i;
    if (juce::MessageManager::existsAndIsCurrentThread()) loadFactoryPreset (i);
    else juce::MessageManager::callAsync ([this, i] { loadFactoryPreset (i); });
}

// ===================================================================================== randomiser
void KreiselProcessor::randomise (int track)
{
    undo.beginNewTransaction ("Zufall");
    auto rt = randTree();
    juce::Random r;
    auto range = [&] (int i, float& lo, float& hi) {
        const juce::String n (randNames[i]);
        lo = (float) rt[n + "_min"]; hi = (float) rt[n + "_max"];
        if (hi < lo) std::swap (lo, hi);
        return (bool) rt[n + "_on"];
    };
    const char* ids[kRandTargets] = { "steps", "beats", "rotate", "chance", "vel", "swing", "pitch", "chord", "clock", "gate" };
    for (int t = 0; t < kTracks; ++t)
    {
        if (track >= 0 && t != track) continue;
        if (track < 0 && getParam (pid (t, "active")) < 0.5f) continue;
        int steps = (int) getParam (pid (t, "steps"));
        for (int i = 0; i < kRandTargets; ++i)
        {
            float lo, hi;
            if (! range (i, lo, hi)) continue;
            float v = lo + r.nextFloat() * (hi - lo);
            const bool isInt = i != tChance && i != tSwing && i != kTargets;
            if (isInt) v = (float) juce::roundToInt (v);
            if (i == tSteps) steps = (int) v;
            if (i == tBeats) v = juce::jmin (v, (float) steps);
            if (i == tRotate) v = juce::jmin (v, (float) juce::jmax (0, steps - 1));
            if (i == tPitch && getParam ("quant") > 0.5f)
                v = (float) snapToScale ((int) v, (int) getParam ("root"), (int) getParam ("scale"));
            setParam (pid (t, ids[i]), v);
        }
    }
}

void KreiselProcessor::randomiseLane (int track, int target)
{
    undo.beginNewTransaction (juce::String::fromUTF8 ("Lane würfeln"));
    std::array<float, kMaxSteps> v {};
    juce::Random r;
    for (auto& x : v) x = r.nextFloat() * 2.0f - 1.0f;
    auto lt = laneTree (track, target);
    lt.setProperty (idVals, valuesToString (v), &undo);
    lt.setProperty (idOn, true, &undo);
}

// ===================================================================================== export
double KreiselProcessor::exportBpm() const
{
    return transportRunning ? currentBpm.load() : (double) pBpm->load();
}

juce::File KreiselProcessor::exportMidiToTemp (int bars)
{
    auto name = currentPresetName().replaceCharacters ("\\/:*?\"<>|", "_________");
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Kreisel")
                 .getChildFile ("Kreisel " + name + " " + juce::String (bars) + (bars == 1 ? " Takt" : " Takte") + ".mid");
    f.getParentDirectory().createDirectory();
    writeMidiFile (renderMidi (bars), f);
    return f;
}

// ===================================================================================== audio
void KreiselProcessor::releaseResources()
{
    const juce::ScopedLock sl (hostLock);
    for (auto& tr : slots)
        for (auto& s2 : tr)
            if (s2.inst != nullptr) s2.inst->releaseResources();
}

void KreiselProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (64, samplesPerBlock);
    engine.prepare (sampleRate);
    trackBuf.setSize (2, juce::jmax (4096, maxBlock));
    for (auto& m : trackMidi) m.ensureSize (8192);
    emptyMidi.ensureSize (256);
    {
        const juce::ScopedLock sl (hostLock);
        for (auto& tr : slots)
            for (auto& sl2 : tr)
                if (sl2.inst != nullptr)
                {
                    sl2.inst->setRateAndBufferSizeDetails (sampleRate, maxBlock);
                    sl2.inst->prepareToPlay (sampleRate, maxBlock);
                    sl2.buf.setSize (juce::jmax (2, sl2.inst->getTotalNumInputChannels(), sl2.inst->getTotalNumOutputChannels()), maxBlock);
                }
    }
    seq.reset();
    pendingOffs.clear();
    lastToQ = -1.0e12;
}

void KreiselProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buffer.clear();
    midi.clear();
    const int numSamples = buffer.getNumSamples();
    const bool intern = pIntern->load() > 0.5f;
    const float master = juce::Decibels::decibelsToGain (pVol->load());

    bool hostPlaying = false;
    double ppq = 0.0, bpm = pBpm->load();
    juce::AudioPlayHead::TimeSignature sig;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto ts = pos->getTimeSignature()) sig = *ts;
            if (pos->getIsPlaying())
                if (auto p = pos->getPpqPosition()) { hostPlaying = true; ppq = *p; }
        }

    hostTransport = hostPlaying;
    const bool internalOn = ! hostPlaying && pRun->load() > 0.5f;
    if (internalOn)
    {
        bpm = pBpm->load();
        if (! internalWasOn) internalPpq = 0.0;
        ppq = internalPpq;
    }
    internalWasOn = internalOn;
    const bool running = hostPlaying || internalOn;
    bpm = juce::jlimit (1.0, 999.0, bpm);

    // transport for hosted plug-ins (the DAW's, or Kreisel's own clock)
    {
        juce::AudioPlayHead::PositionInfo info;
        info.setIsPlaying (running);
        info.setBpm (bpm);
        info.setTimeSignature (sig);
        info.setPpqPosition (ppq);
        info.setTimeInSeconds (ppq * 60.0 / bpm);
        info.setTimeInSamples ((juce::int64) (ppq * 60.0 / bpm * sr));
        innerPlayHead.info = info;
    }

    fillSnapshot (audioSnap);
    outEvents.clear();
    auto flushAllOffs = [&] {
        for (auto& o : pendingOffs)
            outEvents.push_back ({ 0, true, o.ch, o.note, 0, o.track });
        pendingOffs.clear();
        engine.releaseAll();
    };

    if (! running)
    {
        if (wasRunning)
        {
            flushAllOffs();
            seq.reset();
            for (auto& v : vis) v.pos = -1;
        }
        wasRunning = false;
        transportRunning = false;
        lastToQ = -1.0e12;
    }
    else
    {
        const double samplesPerQ = sr * 60.0 / bpm;
        const double fromQ = ppq;
        const double toQ = ppq + numSamples / samplesPerQ;
        if (internalOn) internalPpq = toQ;

        if (std::abs (fromQ - lastToQ) > 1.0e-4 && ! pendingOffs.empty())
            flushAllOffs(); // loop / relocation
        lastToQ = toQ;
        wasRunning = true;
        transportRunning = true;
        currentPpq = fromQ;
        currentBpm = bpm;

        auto toSample = [&] (double q) { return juce::jlimit (0, numSamples - 1, (int) std::floor ((q - fromQ) * samplesPerQ)); };

        seq.process (audioSnap, fromQ, toQ, rng,
            [&] (int t, const ResolvedStep& st, double) {
                auto& v = vis[(size_t) t];
                v.pos = st.pos; v.steps = st.steps; v.beats = st.beats; v.rotate = st.rotate; v.k = st.k;
                v.stepsFired = v.stepsFired + 1;
            },
            [&] (const NoteEvent& e) {
                const int s = toSample (e.q);
                auto& v = vis[(size_t) e.track];
                v.hits = v.hits + 1;
                v.lastVelocity = e.velocity;
                for (int i = 0; i < e.numNotes; ++i)
                {
                    const int note = e.notes[i];
                    for (size_t j = 0; j < pendingOffs.size(); ++j)
                        if (pendingOffs[j].ch == e.channel && pendingOffs[j].note == note)
                        {
                            outEvents.push_back ({ s, true, e.channel, note, 0, pendingOffs[j].track });
                            pendingOffs.erase (pendingOffs.begin() + (long) j);
                            break;
                        }
                    bool dup = false; // same note twice in one chord
                    for (int k = 0; k < i; ++k) dup |= e.notes[k] == note;
                    if (dup) continue;
                    outEvents.push_back ({ s, false, e.channel, note, e.velocity, e.track });
                    if (pendingOffs.size() < 2000)
                        pendingOffs.push_back ({ e.q + e.lenQ, e.channel, note, e.track });
                }
            });

        for (size_t j = 0; j < pendingOffs.size();)
        {
            if (pendingOffs[j].q < toQ)
            {
                outEvents.push_back ({ toSample (pendingOffs[j].q), true, pendingOffs[j].ch, pendingOffs[j].note, 0, pendingOffs[j].track });
                pendingOffs.erase (pendingOffs.begin() + (long) j);
            }
            else ++j;
        }
    }

    std::stable_sort (outEvents.begin(), outEvents.end(), [] (const OutEvent& a, const OutEvent& b) {
        if (a.sample != b.sample) return a.sample < b.sample;
        return a.off && ! b.off;
    });
    for (auto& e : outEvents)
        midi.addEvent (e.off ? juce::MidiMessage::noteOff (e.ch, e.note)
                             : juce::MidiMessage::noteOn (e.ch, e.note, (juce::uint8) e.vel), e.sample);

    mixTracks (buffer, numSamples, intern, master); // built-in sounds and hosted plug-ins
}

// ===================================================================================== state
void KreiselProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("HOSTED"), nullptr);
    state.appendChild (hostedStateTree(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void KreiselProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            // projects saved before the built-in sounds existed keep playing MIDI only
            bool hasIntern = false;
            for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
                hasIntern |= e->getStringAttribute ("id") == "intern";
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            if (! hasIntern) setParam ("intern", 0.0f);
            auto hosted = apvts.state.getChildWithName ("HOSTED").createCopy();
            apvts.state.removeChild (apvts.state.getChildWithName ("HOSTED"), nullptr);
            if (juce::MessageManager::existsAndIsCurrentThread()) restoreHosted (hosted);
            else juce::MessageManager::callAsync ([this, hosted] { restoreHosted (hosted); });
            ensureTrees();
            syncAllLanes();
        }
}

juce::AudioProcessorEditor* KreiselProcessor::createEditor() { return new KreiselEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KreiselProcessor(); }
