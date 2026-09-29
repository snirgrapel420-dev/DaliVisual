#pragma once
// ============================================================================
//  Editor chrome: HeaderBar (logo, scene, presets, fullscreen, settings),
//  MeterBar (Bass · Mid · High · Energy · Beat · BPM · FPS · CPU) and the
//  SettingsPanel overlay (output display, resolution, v-sync, MIDI).
// ============================================================================
#include "ParamControls.h"
#include "DaliLookAndFeel.h"

namespace dali
{
class HeaderBar : public juce::Component, private juce::ChangeListener
{
public:
    explicit HeaderBar(DaliVisualProcessor& p);
    ~HeaderBar() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    std::function<void()> onSettings;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refreshPresets(); updateFullscreenButton(); }
    void refreshPresets();
    void updateFullscreenButton();
    void showPresetMenu();
    void showFullscreenMenu();
    void askName(const juce::String& title, const juce::String& initial, std::function<void(juce::String)> done);

    DaliVisualProcessor& proc;
    ParamCombo scene;
    juce::ComboBox preset;
    juce::TextButton prev { "<" }, next { ">" }, presetMenu { "PRESET" }, fullscreen { "FULLSCREEN" },
                     displayMenu { juce::String::fromUTF8("\xe2\x96\xbe") }, settings { "SETTINGS" };
    juce::Label sceneLabel, presetLabel;
};

class MeterBar : public juce::Component, private juce::Timer
{
public:
    explicit MeterBar(DaliVisualProcessor& p) : proc(p) { startTimerHz(30); }
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    DaliVisualProcessor& proc;
    float bass = 0, mid = 0, high = 0, energy = 0, beat = 0;
    float bpm = 0, fps = 0, outFps = 0, cpu = 0, frameMs = 0;
    int source = 2;
    bool output = false, silent = true;
};

class SettingsPanel : public juce::Component, private juce::Timer
{
public:
    explicit SettingsPanel(DaliVisualProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override { if (isVisible()) refresh(); }
    std::function<void()> onClose;
private:
    void timerCallback() override;
    void refresh();
    DaliVisualProcessor& proc;
    SectionLabel outHeader { "Output" }, midiHeader { "MIDI" }, infoHeader { "System" };
    juce::ComboBox display, resolution;
    juce::Label displayLabel, resolutionLabel, midiLast, info;
    juce::ToggleButton vsync { "V-Sync (locks to the display: 60 / 120 Hz)" },
                       previewWhileOutput { "Keep preview running while fullscreen" },
                       noteScenes { "Notes C1-G1 select scenes 1-8" },
                       programScenes { "Program Change selects scenes" };
    juce::TextButton openOutput { "Open Fullscreen Output" }, clearMidi { "Clear all MIDI mappings" },
                     factory { "Reinstall factory presets" }, close { "Close" };
};
} // namespace dali
