// Kreisel – settings shared by every Kreisel instance and the standalone app:
// VST3 folders and the list of scanned plug-ins (stored in %APPDATA%\Kreisel\Kreisel.settings).
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace kr
{
class HostSettings
{
public:
    HostSettings();
    ~HostSettings();

    juce::KnownPluginList list;
    juce::AudioPluginFormatManager formats;
    juce::StringArray folders;

    void save();
    void reload();                       // re-read the file (another instance may have scanned)
    juce::File settingsFolder() const;
    juce::File deadMansPedal() const;     // crash protection while scanning
    juce::Array<juce::PluginDescription> plugins (bool instruments) const; // sorted, Kreisel itself left out
    static juce::StringArray defaultFolders();

    // helper program that scans one plug-in in its own process (next to the standalone exe,
    // or in the Resources folder of the Kreisel.vst3 bundle)
    static juce::File scannerExe();

    // properties of the loupe etc. that should survive across projects
    juce::PropertiesFile& props() { return *propsFile; }

private:
    std::unique_ptr<juce::PropertiesFile> propsFile;
    JUCE_DECLARE_NON_COPYABLE (HostSettings)
};
} // namespace kr
