#pragma once
// ============================================================================
//  Side-panel pages: SCENE · AUDIO · MOD · FX · COLOR · IMAGE
// ============================================================================
#include "ParamControls.h"
#include "DaliLookAndFeel.h"

namespace dali
{
class PanelBase : public juce::Component
{
public:
    virtual int preferredHeight(int width) = 0;
};

/** Hosts a PanelBase in a vertical scroller. */
class ScrollPanel : public juce::Component
{
public:
    explicit ScrollPanel(std::unique_ptr<PanelBase> c) : content(std::move(c))
    {
        viewport.setViewedComponent(content.get(), false);
        viewport.setScrollBarsShown(true, false);
        viewport.setScrollBarThickness(8);
        addAndMakeVisible(viewport);
    }
    void resized() override
    {
        viewport.setBounds(getLocalBounds());
        const int w = getWidth() - viewport.getScrollBarThickness();
        content->setSize(w, juce::jmax(getHeight(), content->preferredHeight(w)));
    }
    void paint(juce::Graphics& g) override { g.fillAll(colours::panel); }
    PanelBase& getContent() { return *content; }
private:
    std::unique_ptr<PanelBase> content;
    juce::Viewport viewport;
};

// ---------------------------------------------------------------------------------------------
class ScenePanel : public PanelBase, private juce::AudioProcessorValueTreeState::Listener, private juce::AsyncUpdater
{
public:
    explicit ScenePanel(DaliVisualProcessor& p);
    ~ScenePanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    void parameterChanged(const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;
    DaliVisualProcessor& proc;
    SectionLabel sceneHeader { "Scene" }, macroHeader { "Scene Parameters" }, globalHeader { "Global" };
    juce::OwnedArray<juce::TextButton> sceneButtons;
    juce::Label description;
    ParamKnob macroA, macroB, macroC, macroD, intensity, speed;
};

// ---------------------------------------------------------------------------------------------
class AudioPanel : public PanelBase, private juce::Timer
{
public:
    explicit AudioPanel(DaliVisualProcessor& p);
    int preferredHeight(int width) override;
    void resized() override;
private:
    void timerCallback() override;
    DaliVisualProcessor& proc;
    SectionLabel inputHeader { "Analysis" }, reactHeader { "Reaction" }, syncHeader { "Sync" };
    ParamKnob sensitivity, smoothing, bass, mid, high, transient, internalBpm;
    ParamCombo syncSource, syncDiv;
    juce::Label syncSourceLabel, syncDivLabel, readout;
};

// ---------------------------------------------------------------------------------------------
class ModPanel : public PanelBase, private juce::ChangeListener
{
public:
    explicit ModPanel(DaliVisualProcessor& p);
    ~ModPanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    class Row;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    DaliVisualProcessor& proc;
    SectionLabel header { "Modulation Matrix  |  any source > any target" };
    juce::TextButton clearAll { "Clear All" };
    juce::OwnedArray<Row> rows;
};

// ---------------------------------------------------------------------------------------------
class FxPanel : public PanelBase, private juce::ChangeListener
{
public:
    explicit FxPanel(DaliVisualProcessor& p);
    ~FxPanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    class Row;
    void changeListenerCallback(juce::ChangeBroadcaster*) override { resized(); }
    DaliVisualProcessor& proc;
    SectionLabel header { "Effects Rack  |  top > bottom" };
    juce::TextButton resetOrder { "Reset Order" };
    juce::OwnedArray<Row> rows;       // indexed by effect id (library order)
};

// ---------------------------------------------------------------------------------------------
class ColorPanel : public PanelBase
{
public:
    explicit ColorPanel(DaliVisualProcessor& p);
    ~ColorPanel() override;
    int preferredHeight(int width) override;
    void resized() override;
private:
    class Swatch;
    DaliVisualProcessor& proc;
    SectionLabel paletteHeader { "Palette" }, gradeHeader { "Colour" }, customHeader { "Custom Palette" };
    juce::OwnedArray<Swatch> swatches;
    ParamKnob hue, saturation, brightness, contrast, colorAmount, colorShift, audioColor, customA, customB;
};

// ---------------------------------------------------------------------------------------------
class ImagePanel : public PanelBase, private juce::ChangeListener
{
public:
    explicit ImagePanel(DaliVisualProcessor& p);
    ~ImagePanel() override;
    int preferredHeight(int width) override;
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); status.setText(proc.image.getStatus(), juce::dontSendNotification); }
    void chooseImage();
    void saveTemplate();
    void loadTemplate();

    DaliVisualProcessor& proc;
    SectionLabel sourceHeader { "Source Image" }, templateHeader { "Template" }, structureHeader { "Structure" };
    juce::Rectangle<int> dropZone;
    juce::Label status;
    juce::TextButton loadBtn { "Load Image" }, clearBtn { "Clear" }, saveTpl { "Save Template" },
                     loadTpl { "Load Template" }, resetTpl { "Reset" }, routesBtn { "Add Audio Routes" };
    ParamToggle enable, mirror, kaleido;
    ParamCombo mode, blend;
    juce::OwnedArray<ParamKnob> knobs;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace dali
