#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Theme.h"
#include "Loupe.h"

class KreiselEditor;

// ------------------------------------------------------------------------------------------ look & feel
class KLook : public juce::LookAndFeel_V4
{
public:
    void setTheme (const kr::Theme& t);
    const kr::Theme& theme() const { return th; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.0f)); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return juce::Font (juce::FontOptions (juce::jmin (14.0f, h * 0.55f))); }
    juce::Font getLabelFont (juce::Label&) override { return juce::Font (juce::FontOptions (12.5f)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (14.0f)); }
    juce::Label* createSliderTextBox (juce::Slider&) override;

private:
    kr::Theme th = kr::themes()[0];
};

// ------------------------------------------------------------------------------------------ visualizer
class Visualizer : public juce::Component
{
public:
    Visualizer (KreiselProcessor& p, KreiselEditor& e) : proc (p), ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void tick();   // called from editor timer

    int mode = 1;
    std::array<float, kr::kTracks> flash {};
private:
    struct TrackView { bool active, muted; int steps, beats, rotate, pos; float flash; juce::Colour col; double stepQ; };
    std::array<TrackView, kr::kTracks> gather() const;
    KreiselProcessor& proc;
    KreiselEditor& ed;
    std::array<juce::uint32, kr::kTracks> lastHits {};
    float radiusFor (int i, float R0, float R) const { return R0 + (R - R0) * ((float) i + 0.5f) / (float) kr::kTracks; }
};

// ------------------------------------------------------------------------------------------ lane editor
class LaneEditor : public juce::Component
{
public:
    LaneEditor (KreiselProcessor& p, KreiselEditor& e) : proc (p), ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void drawAt (juce::Point<float> p, bool reset);
    KreiselProcessor& proc;
    KreiselEditor& ed;
    std::array<float, kr::kMaxSteps> work {};
    bool dragging = false;
    juce::Point<float> lastPt;
};

// ------------------------------------------------------------------------------------------ MIDI drag source
class MidiDragArea : public juce::Component
{
public:
    MidiDragArea (KreiselProcessor& p, KreiselEditor& e) : proc (p), ed (e) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
private:
    KreiselProcessor& proc;
    KreiselEditor& ed;
    bool dragging = false;
};

// ------------------------------------------------------------------------------------------ matrix view
class MatrixView : public juce::Component
{
public:
    MatrixView (KreiselProcessor& p, KreiselEditor& e);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    KreiselProcessor& proc;
    KreiselEditor& ed;
    static constexpr int kCols = 14;
    juce::OwnedArray<juce::Component> cells;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
};

// ------------------------------------------------------------------------------------------ overlays
class RandomPanel : public juce::Component
{
public:
    RandomPanel (KreiselProcessor& p, KreiselEditor& e);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override {}
    void refresh();
private:
    KreiselProcessor& proc;
    KreiselEditor& ed;
    juce::OwnedArray<juce::ToggleButton> enables;
    juce::OwnedArray<juce::Slider> ranges;
    juce::TextButton rollTrack { juce::String::fromUTF8 ("Spur würfeln") }, rollAll { juce::String::fromUTF8 ("Alle Spuren würfeln") }, close { juce::String::fromUTF8 ("Schließen") };
};

class HelpPanel : public juce::Component
{
public:
    explicit HelpPanel (KreiselEditor& e);
    void paint (juce::Graphics&) override;
    void resized() override;
    void showPage (int page); // 0 = Anleitung, 1 = Info
    static constexpr const char* kWebsiteUrl = "https://www.undabtanzbar.de";
    static constexpr const char* kDonateUrl = "https://paypal.me/yakobo";
    static constexpr const char* kGithubUrl = "https://github.com/Yakobo1970/Kreisel";
private:
    KreiselEditor& ed;
    juce::TextEditor text;
    juce::String guide, info;
    juce::TextButton tabGuide { "Anleitung" }, tabInfo { "Info" };
    juce::TextButton donate { juce::String::fromUTF8 ("♥  Spenden via PayPal") }, website { "www.undabtanzbar.de" }, github { "GitHub" };
    juce::TextButton close { juce::String::fromUTF8 ("Schließen") };
};

class SettingsPanel : public juce::Component, private juce::Timer
{
public:
    SettingsPanel (KreiselProcessor& p, KreiselEditor& e);
    ~SettingsPanel() override;
    // for the test tool
    void scan (bool full) { startScan (full); }
    bool isScanning() const { return scanning; }
    juce::String statusText() const { return status.getText(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override {}

private:
    void timerCallback() override;
    void startScan (bool full);
    void refresh();
    void refreshLists();

    struct FolderModel : juce::ListBoxModel
    {
        explicit FolderModel (SettingsPanel& o) : owner (o) {}
        int getNumRows() override;
        void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
        SettingsPanel& owner;
    };
    struct PluginModel : juce::ListBoxModel
    {
        explicit PluginModel (SettingsPanel& o) : owner (o) {}
        int getNumRows() override;
        void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
        SettingsPanel& owner;
    };

    KreiselProcessor& proc;
    KreiselEditor& ed;
    FolderModel folderModel { *this };
    PluginModel pluginModel { *this };
    juce::ListBox folderList, pluginList;
    juce::TextButton addFolder { juce::String::fromUTF8 ("Ordner hinzufügen …") }, removeFolder { "Ordner entfernen" },
        defaults { "Standardordner" }, scanNew { "Neue Plugins suchen" }, scanAll { "Alle neu scannen" },
        clearList { "Liste leeren" }, close { juce::String::fromUTF8 ("Schließen") };
    juce::Label status;
    double progress = 0.0;
    juce::ProgressBar progressBar { progress };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Array<juce::PluginDescription> shown;

    // out-of-process scan: one KreiselScanner.exe per plug-in file
    bool scanning = false;
    juce::StringArray pending, failed;
    int scanIndex = 0, foundNow = 0;
    std::unique_ptr<juce::ChildProcess> child;
    juce::File childOut;
    double childStart = 0.0;
    void launchNext();
    void finishChild (bool timedOut);
    void endScan();
    void setButtonsEnabled (bool);
};

// ------------------------------------------------------------------------------------------ track tab
// Tab button that also offers renaming (double click) and a context menu (right click).
class TrackTab : public juce::TextButton
{
public:
    std::function<void()> onRename, onMenu;
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onMenu) onMenu(); return; }
        juce::TextButton::mouseDown (e);
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (onRename) onRename(); }
};

// ------------------------------------------------------------------------------------------ editor
class KreiselEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::ValueTree::Listener
{
public:
    explicit KreiselEditor (KreiselProcessor&);
    ~KreiselEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    KLook look;
    const kr::Theme& theme() const { return look.theme(); }
    int selectedTrack() const { return selTrack; }
    int selectedTarget() const { return selTarget; }
    void selectTrack (int t);
    void selectTarget (int g);
    int exportBars() const;
    void showRandomPanel (bool show);
    void showHelp (bool show);
    void showSettings (bool show);
    SettingsPanel* settings() { return settingsPanel.get(); }
    void applyTheme();
    void renameTrack (int t);
    void refreshTrackNames();
    void showTrackMenu (int t);
    void updateLoupe();
    void showLoupeMenu();
    void raiseOverlays();

private:
    void timerCallback() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeRedirected (juce::ValueTree&) override { refreshLaneControls(); refreshPresetLabel(); }
    void attachTrackControls();
    void refreshLaneControls();
    void refreshPresetList();
    void refreshPresetLabel();
    void stepPreset (int delta);
    void setLaneProp (const juce::Identifier& id, const juce::var& v);

    KreiselProcessor& proc;
    int selTrack = 0, selTarget = 0;

    // header
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "Speichern" }, loadPreset { "Laden" };
    juce::ComboBox presetBox;
    juce::TextButton undoBtn { juce::String::fromUTF8 ("Rückgängig") }, redoBtn { "Wiederholen" }, randomBtn { "Zufall" }, helpBtn { "?" };
    juce::TextButton matrixBtn { "Matrix" }, pluginsBtn { "Plugins" }, playBtn;
    juce::ComboBox visualBox, themeBox;
    juce::ToggleButton lightToggle { "Hell" };

    // view
    Visualizer visual { proc, *this };
    std::unique_ptr<MatrixView> matrix;

    // tracks
    juce::OwnedArray<TrackTab> trackTabs;
    juce::StringArray shownNames;
    juce::OwnedArray<juce::TextButton> activeBtns, muteBtns, soloBtns;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> tabAtt;

    struct Knob { juce::Slider s; juce::Label l; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a; juce::String id; };
    juce::OwnedArray<Knob> knobs;
    juce::ComboBox chordBox, clockBox, soundBox;
    juce::Label chordLbl, clockLbl, soundLbl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chordAtt, clockAtt, soundAtt;

    // global
    juce::ComboBox rootBox, scaleBox;
    juce::ToggleButton quantToggle { "Skala quantisieren" }, runToggle { "Interner Lauf" };
    juce::ToggleButton internToggle { juce::String::fromUTF8 ("Interne Klänge") };
    juce::Slider bpmSlider, volSlider;
    juce::Label globalLbl, bpmLbl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootAtt, scaleAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> quantAtt, runAtt, internAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAtt, volAtt;

    // export
    juce::ComboBox barsBox;
    MidiDragArea dragArea { proc, *this };
    juce::TextButton saveMidiBtn { "Als .mid speichern" };
    juce::Label exportLbl;

    // lanes
    juce::OwnedArray<juce::TextButton> targetTabs;
    juce::ToggleButton laneOn { "Lane aktiv" };
    juce::Slider laneLen, laneDepth;
    juce::Label laneLenLbl, laneDepthLbl, laneTitle;
    juce::ComboBox laneDir;
    juce::TextButton laneRandom { juce::String::fromUTF8 ("Würfeln") }, laneClear { "Leeren" }, laneCopyAll { "Auf alle Spuren" };
    LaneEditor laneEditor { proc, *this };

    std::unique_ptr<RandomPanel> randomPanel;
    std::unique_ptr<HelpPanel> helpPanel;
    std::unique_ptr<SettingsPanel> settingsPanel;
    std::unique_ptr<kr::ui::Loupe> loupe;
    std::unique_ptr<kr::ui::RightClickCatcher> catcher;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> rTracks, rKnobs, rGlobal, rExport, rLane, rView;
    bool refreshing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KreiselEditor)
};
