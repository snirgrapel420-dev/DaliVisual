#pragma once
// ============================================================================
//  DALI VISUAL — editor.
//   ┌──────────────── Header: logo · scene · preset · fullscreen · settings ─┐
//   │                                          │ SCENE AUDIO MOD FX COLOR IMG │
//   │            live preview (OpenGL)         │        side panel            │
//   ├──────── Bass · Mid · High · Energy · Beat · BPM · FPS · CPU ───────────┤
//  Drop an image anywhere to start Image Reactive Mode. F = fullscreen.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/DaliLookAndFeel.h"
#include "UI/VisualView.h"
#include "UI/Panels.h"
#include "UI/Chrome.h"

class DaliVisualEditor : public juce::AudioProcessorEditor,
                         public juce::FileDragAndDropTarget
{
public:
    explicit DaliVisualEditor(DaliVisualProcessor&);
    ~DaliVisualEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray&, int, int) override { dragging = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { dragging = false; repaint(); }
    void filesDropped(const juce::StringArray& files, int, int) override;

private:
    enum Tab { SceneTab, AudioTab, ModTab, FxTab, ColorTab, ImageTab };

    DaliVisualProcessor& proc;
    dali::DaliLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };
    dali::HeaderBar header;
    dali::VisualView preview;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    dali::MeterBar meters;
    dali::SettingsPanel settings;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DaliVisualEditor)
};
