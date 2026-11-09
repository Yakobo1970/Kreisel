// Renders the Kreisel editor into a PNG.
// Usage: UiShot <out.png> [visual 0-8] [theme 0-5] [light 0/1] [preset index] [panel: matrix|random|help] [playPpq]
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Presets.h"

class UiShotApp : public juce::JUCEApplication, private juce::Timer
{
public:
    std::unique_ptr<KreiselProcessor> scanProc;
    std::unique_ptr<KreiselEditor> scanEd;
    double scanStart = 0;
    void timerCallback() override
    {
        auto* sp = scanEd->settings();
        if (sp->isScanning()) return;
        stopTimer();
        auto& hs = *scanProc->hostSettings;
        juce::String log;
        log << "status: " << sp->statusText() << "\n";
        log << "seconds: " << juce::String ((juce::Time::getMillisecondCounterHiRes() - scanStart) / 1000.0, 1) << "\n";
        log << "instruments: " << hs.plugins (true).size() << ", effects: " << hs.plugins (false).size() << "\n";
        log << "blacklisted:\n  " << hs.list.getBlacklistedFiles().joinIntoString ("\n  ") << "\n";
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getSiblingFile ("scantest.txt").replaceWithText (log);
        scanEd.reset();
        scanProc.reset();
        quit();
    }
    const juce::String getApplicationName() override { return "UiShot"; }
    const juce::String getApplicationVersion() override { return "1"; }

    void initialise (const juce::String&) override
    {
        const auto args = getCommandLineParameterArray();
        KreiselProcessor proc;
        if (args.size() > 0 && args[0] == "undotest")
        {
            const float before = proc.getParam ("t1_steps");
            const auto laneBefore = proc.getLane (5, kr::tRotate).on;
            proc.loadFactoryPreset (13);   // "Verschiebung": track 3 = 5 steps, rotate lane on
            proc.apvts.copyState();        // flush parameters into the tree (undoable)
            const float t3 = proc.getParam ("t3_steps");
            const bool laneOn = proc.getLane (2, kr::tRotate).on;
            proc.undo.beginNewTransaction();
            proc.randomise (0);
            proc.apvts.copyState();
            proc.undo.undo();              // undo randomise
            proc.undo.undo();              // undo preset
            const float t3After = proc.getParam ("t3_steps");
            const bool laneAfter = proc.getLane (2, kr::tRotate).on;
            const bool ok = t3 == 5.0f && laneOn && t3After == 16.0f && ! laneAfter && before == 16.0f && ! laneBefore;
            juce::File ("undotest.txt").replaceWithText (juce::String ("t3 ") + juce::String (t3) + " -> " + juce::String (t3After)
                + " lane " + juce::String ((int) laneOn) + " -> " + juce::String ((int) laneAfter) + (ok ? "  OK" : "  FAIL"));
            setApplicationReturnValue (ok ? 0 : 1);
            quit();
            return;
        }
        if (args.size() > 0 && args[0] == "render")
        {
            // usage: UiShot render <category> <out-folder> <bars> [bpm]  -> one WAV per preset of that category
            const auto cat = args[1];
            const juce::File folder (args[2]);
            folder.createDirectory();
            const int bars = juce::jmax (1, args[3].getIntValue());
            const double bpm = args.size() > 4 ? args[4].getDoubleValue() : 124.0;
            juce::String log;
            for (int i = 0; i < kr::numFactoryPresets(); ++i)
            {
                if (kr::factoryCategory (i) != cat) continue;
                KreiselProcessor p;
                p.setPlayConfigDetails (0, 2, 48000.0, 512);
                p.prepareToPlay (48000.0, 512);
                p.loadFactoryPreset (i);
                p.setParam ("bpm", (float) bpm);
                p.setParam ("intern", 1.0f);
                p.setParam ("run", 1.0f);
                const int total = (int) (bars * 4 * 60.0 / bpm * 48000.0);
                juce::AudioBuffer<float> all (2, total + 512), buf (2, 512);
                juce::MidiBuffer mb;
                for (int pos = 0; pos < total; pos += 512)
                {
                    p.processBlock (buf, mb);
                    for (int c = 0; c < 2; ++c) all.copyFrom (c, pos, buf, c, 0, 512);
                }
                all.setSize (2, total, true);
                const float peak = all.getMagnitude (0, total);
                const float rms = 0.5f * (all.getRMSLevel (0, 0, total) + all.getRMSLevel (1, 0, total));
                const auto f = folder.getChildFile ("Kreisel Demo - " + kr::factoryName (i) + ".wav");
                f.deleteFile();
                if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
                {
                    juce::WavAudioFormat wav;
                    if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), 48000.0, 2, 24, {}, 0)))
                    {
                        os.release();
                        w->writeFromAudioSampleBuffer (all, 0, total);
                    }
                }
                log << kr::factoryName (i) << ": peak " << juce::Decibels::gainToDecibels (peak) << " dB, rms "
                    << juce::Decibels::gainToDecibels (rms) << " dB\n";
            }
            folder.getChildFile ("render.txt").replaceWithText (log);
            quit();
            return;
        }
        if (args.size() > 0 && args[0] == "scantest")
        {
            // full out-of-process scan through the settings panel, like "Alle neu scannen"
            scanProc = std::make_unique<KreiselProcessor>();
            scanEd.reset (dynamic_cast<KreiselEditor*> (scanProc->createEditor()));
            scanEd->showSettings (true);
            scanEd->settings()->scan (true);
            scanStart = juce::Time::getMillisecondCounterHiRes();
            startTimer (500);
            return;
        }
        if (args.size() > 0 && args[0] == "hosttest")
        {
            // usage: UiShot hosttest <instrument.vst3> <effect.vst3>
            juce::String log;
            bool ok = true;
            auto find = [&] (const juce::String& file, juce::PluginDescription& d) {
                juce::OwnedArray<juce::PluginDescription> found;
                juce::VST3PluginFormat().findAllTypesForFile (found, file);
                if (found.isEmpty()) return false;
                d = *found[0];
                return true;
            };
            juce::PluginDescription inst, fx;
            ok &= find (args[1], inst) && find (args[2], fx);
            log << "found: " << inst.name << " (instrument " << (int) inst.isInstrument << "), " << fx.name << "\n";
            proc.setPlayConfigDetails (0, 2, 48000.0, 512);
            proc.prepareToPlay (48000.0, 512);
            proc.setParam ("run", 1.0f);
            for (int t = 1; t < kr::kTracks; ++t) proc.setParam (kr::pid (t, "active"), 0.0f);
            proc.setParam (kr::pid (0, "chan"), 1.0f);
            proc.setParam (kr::pid (0, "pitch"), 48.0f);
            proc.setParam ("intern", 0.0f); // only the hosted instrument may sound
            auto err = proc.loadHosted (0, 0, inst);
            log << "load instrument: " << (err.isEmpty() ? juce::String ("ok") : err) << "\n";
            err = proc.loadHosted (0, 1, fx);
            log << "load effect: " << (err.isEmpty() ? juce::String ("ok") : err) << "\n";
            ok &= proc.hasHosted (0, 0) && proc.hasHosted (0, 1);
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer mb;
            float peak = 0.0f;
            bool finite = true;
            for (int i = 0; i < 400; ++i)
            {
                proc.processBlock (buf, mb);
                peak = juce::jmax (peak, buf.getMagnitude (0, 512));
                for (int s = 0; s < 512; ++s) finite &= std::isfinite (buf.getSample (0, s));
            }
            log << "peak with hosted instrument + effect: " << peak << " finite " << (int) finite << "\n";
            ok &= peak > 0.001f && finite;

            juce::MemoryBlock state;
            proc.getStateInformation (state);
            {
                KreiselProcessor p2;
                p2.setStateInformation (state.getData(), (int) state.getSize());
                log << "restored: " << p2.hostedName (0, 0) << " + " << p2.hostedName (0, 1) << " (" << (int) state.getSize() << " bytes)\n";
                ok &= p2.hostedName (0, 0) == inst.name && p2.hostedName (0, 1) == fx.name;
            }
            proc.unloadHosted (0, 0);
            proc.unloadHosted (0, 1);
            ok &= ! proc.hasHosted (0, 0);
            log << (ok ? "OK" : "FAIL") << "\n";
            juce::File::getSpecialLocation (juce::File::currentExecutableFile).getSiblingFile ("hosttest.txt").replaceWithText (log);
            setApplicationReturnValue (ok ? 0 : 1);
            quit();
            return;
        }
        auto ui = proc.uiTree();
        if (args.size() > 1) ui.setProperty ("visual", args[1].getIntValue(), nullptr);
        if (args.size() > 2) ui.setProperty ("theme", args[2].getIntValue(), nullptr);
        if (args.size() > 3) ui.setProperty ("light", args[3].getIntValue() != 0, nullptr);
        if (args.size() > 4 && args[4].isNotEmpty() && args[4] != "-") proc.loadFactoryPreset (args[4].getIntValue());
        const juce::String panel = args.size() > 5 ? args[5] : juce::String();
        if (panel == "matrix") ui.setProperty ("matrix", true, nullptr);
        if (args.size() > 6)
        {
            // simulate playback so the playheads/flashes show
            proc.setPlayConfigDetails (0, 2, 48000.0, 512);
            proc.prepareToPlay (48000.0, 512);
            proc.setParam ("run", 1.0f);
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer mb;
            const double q = args[6].getDoubleValue();
            const int blocks = (int) (q * 48000.0 * 0.5 / 512.0);
            for (int i = 0; i < blocks; ++i) proc.processBlock (buf, mb);
        }
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
            auto* ke = dynamic_cast<KreiselEditor*> (ed.get());
            if (panel == "random") ke->showRandomPanel (true);
            if (panel == "help") ke->showHelp (true);
            if (panel == "settings") ke->showSettings (true);
            const auto img = ed->createComponentSnapshot (ed->getLocalBounds());
            const juce::File out { args.isEmpty() ? juce::String ("uishot.png") : args[0] };
            out.deleteFile();
            juce::FileOutputStream os (out);
            juce::PNGImageFormat().writeImageToStream (img, os);
        }
        quit();
    }

    void shutdown() override {}
};

START_JUCE_APPLICATION (UiShotApp)
