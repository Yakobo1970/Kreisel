// Kreisel – hosting VST3 instruments and effects per track, plus the shared audio mix.
#include "PluginProcessor.h"

using namespace kr;

namespace
{
// Window for a hosted plug-in's editor. Closing only hides it; it is deleted with the plug-in.
class HostedWindow : public juce::DocumentWindow
{
public:
    HostedWindow (const juce::String& title, juce::AudioProcessorEditor* ed)
        : juce::DocumentWindow (title, juce::Colour (0xff20232b), juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (ed, true);
        setResizable (ed->isResizable(), false);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }
    void closeButtonPressed() override { setVisible (false); }
};

inline float softClip (float x)
{
    const float a = std::abs (x);
    if (a <= 0.8f) return x;
    return std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), x);
}
} // namespace

void KreiselProcessor::configureLayout (juce::AudioPluginInstance& p, bool effect)
{
    p.enableAllBuses();
    auto layout = p.getBusesLayout();
    for (int i = 0; i < layout.outputBuses.size(); ++i)
        layout.outputBuses.getReference (i) = i == 0 ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::disabled();
    for (int i = 0; i < layout.inputBuses.size(); ++i)
        layout.inputBuses.getReference (i) = (effect && i == 0) ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::disabled();
    if (p.checkBusesLayoutSupported (layout))
        p.setBusesLayout (layout);
}

juce::String KreiselProcessor::loadHosted (int track, int kind, const juce::PluginDescription& desc, const juce::MemoryBlock* state)
{
    if (! juce::isPositiveAndBelow (track, kTracks) || (kind != slotInstrument && kind != slotEffect)) return "Falscher Slot";
    if (desc.name.equalsIgnoreCase ("Kreisel")) return juce::String::fromUTF8 ("Kreisel kann sich nicht selbst laden.");

    const double rate = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const int block = juce::jmax (maxBlock, getBlockSize() > 0 ? getBlockSize() : 512);
    juce::String err;
    auto inst = hostSettings->formats.createPluginInstance (desc, rate, block, err);
    if (inst == nullptr)
        return err.isNotEmpty() ? err : juce::String::fromUTF8 ("Das Plugin ließ sich nicht laden.");

    configureLayout (*inst, kind == slotEffect);
    inst->setPlayHead (&innerPlayHead);
    inst->setNonRealtime (isNonRealtime());
    inst->prepareToPlay (rate, block);
    if (state != nullptr && state->getSize() > 0)
        inst->setStateInformation (state->getData(), (int) state->getSize());

    juce::AudioBuffer<float> buf (juce::jmax (2, inst->getTotalNumInputChannels(), inst->getTotalNumOutputChannels()), block);
    closeHostedEditor (track, kind);
    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (hostLock);
        auto& s = slots[(size_t) track][(size_t) kind];
        old = std::move (s.inst);
        s.inst = std::move (inst);
        s.desc = desc;
        s.buf = std::move (buf);
    }
    if (old != nullptr) old->releaseResources();
    return {};
}

void KreiselProcessor::unloadHosted (int track, int kind)
{
    if (! juce::isPositiveAndBelow (track, kTracks) || ! juce::isPositiveAndBelow (kind, 2)) return;
    closeHostedEditor (track, kind);
    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (hostLock);
        old = std::move (slots[(size_t) track][(size_t) kind].inst);
        slots[(size_t) track][(size_t) kind].desc = {};
    }
    if (old != nullptr) old->releaseResources();
}

bool KreiselProcessor::hasHosted (int track, int kind) const
{
    return juce::isPositiveAndBelow (track, kTracks) && juce::isPositiveAndBelow (kind, 2)
           && slots[(size_t) track][(size_t) kind].inst != nullptr;
}

juce::String KreiselProcessor::hostedName (int track, int kind) const
{
    return hasHosted (track, kind) ? slots[(size_t) track][(size_t) kind].desc.name : juce::String();
}

juce::String KreiselProcessor::hostedId (int track, int kind) const
{
    return hasHosted (track, kind) ? slots[(size_t) track][(size_t) kind].desc.createIdentifierString() : juce::String();
}

void KreiselProcessor::closeHostedEditor (int track, int kind)
{
    auto& w = hostedWindows[(size_t) (track * 2 + kind)];
    if (w != nullptr) w.reset();
}

void KreiselProcessor::showHostedEditor (int track, int kind)
{
    if (! hasHosted (track, kind)) return;
    auto& w = hostedWindows[(size_t) (track * 2 + kind)];
    if (w == nullptr)
    {
        auto* inst = slots[(size_t) track][(size_t) kind].inst.get();
        auto* ed = inst->hasEditor() ? inst->createEditorIfNeeded() : nullptr;
        if (ed == nullptr) ed = new juce::GenericAudioProcessorEditor (*inst);
        w = std::make_unique<HostedWindow> (trackName (track) + juce::String::fromUTF8 (" · ") + inst->getName(), ed);
    }
    w->setVisible (true);
    w->toFront (true);
}

juce::ValueTree KreiselProcessor::hostedStateTree()
{
    juce::ValueTree hosted ("HOSTED");
    for (int t = 0; t < kTracks; ++t)
        for (int k = 0; k < 2; ++k)
        {
            auto& s = slots[(size_t) t][(size_t) k];
            if (s.inst == nullptr) continue;
            juce::ValueTree v ("SLOT");
            v.setProperty ("track", t, nullptr);
            v.setProperty ("kind", k, nullptr);
            if (auto x = s.desc.createXml())
                v.setProperty ("desc", x->toString (juce::XmlElement::TextFormat().singleLine()), nullptr);
            juce::MemoryBlock mb;
            s.inst->getStateInformation (mb);
            v.setProperty ("state", mb.toBase64Encoding(), nullptr);
            hosted.appendChild (v, nullptr);
        }
    return hosted;
}

void KreiselProcessor::restoreHosted (const juce::ValueTree& hosted)
{
    for (int t = 0; t < kTracks; ++t)
        for (int k = 0; k < 2; ++k)
            unloadHosted (t, k);
    for (auto v : hosted)
    {
        juce::PluginDescription d;
        auto x = juce::parseXML (v.getProperty ("desc").toString());
        if (x == nullptr || ! d.loadFromXml (*x)) continue;
        juce::MemoryBlock mb;
        mb.fromBase64Encoding (v.getProperty ("state").toString());
        loadHosted ((int) v.getProperty ("track"), (int) v.getProperty ("kind"), d, &mb);
    }
}

// ===================================================================================== mixing (audio thread)
void KreiselProcessor::mixTracks (juce::AudioBuffer<float>& out, int numSamples, bool intern, float master)
{
    const juce::ScopedTryLock sl (hostLock);
    const bool hostOk = sl.isLocked();
    const int chans = out.getNumChannels();
    if (chans == 0) return;

    for (int t = 0; t < kTracks; ++t)
    {
        auto* inst = hostOk && slots[(size_t) t][0].inst != nullptr && numSamples <= slots[(size_t) t][0].buf.getNumSamples() ? &slots[(size_t) t][0] : nullptr;
        auto* fx = hostOk && slots[(size_t) t][1].inst != nullptr && numSamples <= slots[(size_t) t][1].buf.getNumSamples() ? &slots[(size_t) t][1] : nullptr;
        const auto& tr = audioSnap.tracks[(size_t) t];
        const float* L = nullptr;
        const float* R = nullptr;

        if (inst != nullptr)
        {
            auto& midiT = trackMidi[(size_t) t];
            midiT.clear();
            for (auto& e : outEvents)
                if (e.track == t)
                    midiT.addEvent (e.off ? juce::MidiMessage::noteOff (e.ch, e.note)
                                          : juce::MidiMessage::noteOn (e.ch, e.note, (juce::uint8) e.vel), e.sample);
            auto& b = inst->buf;
            juce::AudioBuffer<float> view (b.getArrayOfWritePointers(), b.getNumChannels(), numSamples);
            view.clear();
            inst->inst->processBlock (view, midiT);
            L = b.getReadPointer (0);
            R = b.getNumChannels() > 1 ? b.getReadPointer (1) : L;
        }
        else if (intern && numSamples <= trackBuf.getNumSamples())
        {
            float* tl = trackBuf.getWritePointer (0);
            float* trr = trackBuf.getWritePointer (1);
            std::fill (tl, tl + numSamples, 0.0f);
            std::fill (trr, trr + numSamples, 0.0f);
            int done = 0;
            for (auto& e : outEvents)
            {
                if (e.track != t) continue;
                if (e.sample > done) { engine.render (tl + done, trr + done, e.sample - done, t); done = e.sample; }
                if (e.off) engine.noteOff (t, e.note);
                else engine.noteOn (t, resolveSound (tr), e.note, e.vel, tr.level / 100.0f);
            }
            engine.render (tl + done, trr + done, numSamples - done, t);
            L = tl;
            R = trr;
        }
        if (L == nullptr) continue;

        if (fx != nullptr)
        {
            auto& fb = fx->buf;
            juce::AudioBuffer<float> view (fb.getArrayOfWritePointers(), fb.getNumChannels(), numSamples);
            view.clear();
            view.copyFrom (0, 0, L, numSamples);
            if (view.getNumChannels() > 1) view.copyFrom (1, 0, R, numSamples);
            emptyMidi.clear();
            fx->inst->processBlock (view, emptyMidi);
            L = fb.getReadPointer (0);
            R = fb.getNumChannels() > 1 ? fb.getReadPointer (1) : L;
        }

        const float g = master * (inst != nullptr ? tr.level / 100.0f : 1.0f);
        if (chans == 1)
        {
            out.addFrom (0, 0, L, numSamples, 0.5f * g);
            out.addFrom (0, 0, R, numSamples, 0.5f * g);
        }
        else
        {
            out.addFrom (0, 0, L, numSamples, g);
            out.addFrom (1, 0, R, numSamples, g);
        }
    }

    for (int c = 0; c < chans; ++c)
    {
        auto* d = out.getWritePointer (c);
        for (int i = 0; i < numSamples; ++i) d[i] = softClip (d[i]);
    }
}
