#include "HostSettings.h"

namespace kr
{
HostSettings::HostSettings()
{
    formats.addFormat (std::make_unique<juce::VST3PluginFormat>());
    juce::PropertiesFile::Options o;
    o.applicationName = "Kreisel";
    o.filenameSuffix = "settings";
    o.folderName = "Kreisel";
    o.osxLibrarySubFolder = "Application Support";
    o.commonToAllUsers = false;
    o.storageFormat = juce::PropertiesFile::storeAsXML;
    propsFile = std::make_unique<juce::PropertiesFile> (o);
    reload();
}

HostSettings::~HostSettings() = default;

juce::StringArray HostSettings::defaultFolders()
{
    juce::StringArray a;
    const auto path = juce::VST3PluginFormat().getDefaultLocationsToSearch();
    for (int i = 0; i < path.getNumPaths(); ++i)
        a.addIfNotAlreadyThere (path.getRawString (i));
    return a;
}

juce::File HostSettings::scannerExe()
{
    const auto self = juce::File::getSpecialLocation (juce::File::currentExecutableFile); // the DLL inside a plug-in
    const juce::File candidates[] = {
        self.getSiblingFile ("KreiselScanner.exe"),
        self.getParentDirectory().getParentDirectory().getChildFile ("Resources").getChildFile ("KreiselScanner.exe"),
        juce::File ("C:\\Program Files\\Common Files\\VST3\\Kreisel.vst3\\Contents\\Resources\\KreiselScanner.exe")
    };
    for (auto& f : candidates)
        if (f.existsAsFile()) return f;
    return {};
}

void HostSettings::reload()
{
    propsFile->reload();
    folders = juce::StringArray::fromTokens (propsFile->getValue ("folders"), "|", "");
    folders.removeEmptyStrings();
    if (folders.isEmpty()) folders = defaultFolders();
    if (auto xml = propsFile->getXmlValue ("plugins"))
        list.recreateFromXml (*xml);
}

void HostSettings::save()
{
    propsFile->setValue ("folders", folders.joinIntoString ("|"));
    if (auto xml = list.createXml())
        propsFile->setValue ("plugins", xml.get());
    propsFile->saveIfNeeded();
}

juce::File HostSettings::settingsFolder() const
{
    return propsFile->getFile().getParentDirectory();
}

juce::File HostSettings::deadMansPedal() const
{
    return settingsFolder().getChildFile ("scan-in-progress.txt");
}

juce::Array<juce::PluginDescription> HostSettings::plugins (bool instruments) const
{
    juce::Array<juce::PluginDescription> out;
    for (auto& d : list.getTypes())
        if (d.isInstrument == instruments && ! d.name.equalsIgnoreCase ("Kreisel"))
            out.add (d);
    std::sort (out.begin(), out.end(), [] (const juce::PluginDescription& a, const juce::PluginDescription& b) {
        const int m = a.manufacturerName.compareIgnoreCase (b.manufacturerName);
        return m != 0 ? m < 0 : a.name.compareIgnoreCase (b.name) < 0;
    });
    return out;
}
} // namespace kr
