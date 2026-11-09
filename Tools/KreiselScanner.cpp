// Kreisel plug-in scanner: inspects ONE plug-in file in its own process, so a plug-in that crashes
// or hangs while being scanned cannot take Kreisel (or the DAW) down with it.
// Usage: KreiselScanner <plugin.vst3> <out.xml>   -> writes <SCAN><PLUGIN .../>...</SCAN>, exit code 0 = ok
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <juce_audio_processors/juce_audio_processors.h>

int wmain (int argc, wchar_t** argv) // wide arguments: plug-in paths may contain umlauts
{
    // no "program has stopped working" dialogs: a crash simply ends this process
    SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    if (argc < 3) return 2;
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String file (argv[1]);
    const juce::File out ((juce::String (argv[2])));

    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> found;
    format.findAllTypesForFile (found, file);

    juce::XmlElement xml ("SCAN");
    for (auto* d : found)
        if (auto e = d->createXml())
            xml.addChildElement (e.release());
    if (! xml.writeTo (out)) return 3;
    return found.isEmpty() ? 1 : 0;
}
