; Kreisel Setup - Inno Setup Skript
;
; Installiert das VST3-Plugin nach C:\Program Files\Common Files\VST3 und das
; Standalone-Programm nach C:\Program Files\Kreisel (beides waehlbar).
;
; Vorher Release bauen:
;   cmake --build build --config Release --target Kreisel_VST3 Kreisel_Standalone
; Dann:
;   "C:\Program Files\Inno Setup 7\ISCC.exe" installer\Kreisel.iss

#define AppName "Kreisel"
#define AppVersion "1.1.0"
#define AppPublisher "Kay 'Yakobo' Fiedler (Yakobo1970)"
#define AppURL "https://www.undabtanzbar.de/software/kreisel/"
#define AppExeName "Kreisel.exe"
#define BuildDir "..\build\Kreisel_artefacts\Release"

[Setup]
AppId={{6C1F4E2A-93B7-4D58-A0E3-5B7C2D9F4A61}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL=https://github.com/Yakobo1970/Kreisel/releases
AppComments=Euklidischer MIDI-Sequencer - VST3 und Standalone
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#AppExeName}
OutputDir=..\dist_installer
OutputBaseFilename=Kreisel-{#AppVersion}-Setup
Compression=lzma2
SolidCompression=yes
; Der VST3-Ordner unter Common Files braucht Administratorrechte.
PrivilegesRequired=admin
LicenseFile=..\LICENSE
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
SetupIconFile=Kreisel.ico

[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "VST3 + Standalone"
Name: "custom"; Description: "Benutzerdefiniert"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3-Plugin (C:\Program Files\Common Files\VST3)"; Types: full custom
Name: "standalone"; Description: "Standalone-Programm"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: standalone

[Files]
Source: "{#BuildDir}\VST3\Kreisel.vst3\*"; DestDir: "{commoncf64}\VST3\Kreisel.vst3"; Components: vst3; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "{#BuildDir}\Standalone\{#AppExeName}"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#BuildDir}\Standalone\KreiselScanner.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Components: standalone
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent; Components: standalone
