#include "PluginEditor.h"

DaliVisualEditor::DaliVisualEditor(DaliVisualProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
      header(p), preview(p.engineState, dali::RenderEngine::Role::Preview), meters(p), settings(p)
{
    setLookAndFeel(&lnf);

    addAndMakeVisible(header);
    addAndMakeVisible(preview);
    addAndMakeVisible(tabs);
    addAndMakeVisible(meters);
    addChildComponent(settings);

    auto add = [this](const juce::String& name, std::unique_ptr<dali::PanelBase> panel)
    {
        tabs.addTab(name, dali::colours::panel, new dali::ScrollPanel(std::move(panel)), true);
    };
    add("SCENE", std::make_unique<dali::ScenePanel>(p));
    add("AUDIO", std::make_unique<dali::AudioPanel>(p));
    add("MOD",   std::make_unique<dali::ModPanel>(p));
    add("FX",    std::make_unique<dali::FxPanel>(p));
    add("COLOR", std::make_unique<dali::ColorPanel>(p));
    add("IMAGE", std::make_unique<dali::ImagePanel>(p));
    tabs.setTabBarDepth(32);
    tabs.setOutline(0);

    header.onSettings = [this] { settings.setVisible(!settings.isVisible()); if (settings.isVisible()) settings.toFront(true); };
    settings.onClose = [this] { settings.setVisible(false); };
    preview.onDoubleClick = [this] { proc.output.toggle(); };

    setWantsKeyboardFocus(true);
    setResizable(true, true);
    setResizeLimits(1060, 660, 3840, 2160);
    setSize(1320, 800);
}

DaliVisualEditor::~DaliVisualEditor()
{
    setLookAndFeel(nullptr);
}

void DaliVisualEditor::paint(juce::Graphics& g)
{
    g.fillAll(dali::colours::bg);
}

void DaliVisualEditor::paintOverChildren(juce::Graphics& g)
{
    if (!dragging) return;
    g.setColour(dali::colours::accent.withAlpha(0.18f));
    g.fillRect(getLocalBounds());
    g.setColour(dali::colours::accent);
    g.drawRect(getLocalBounds(), 3);
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawText("Drop image to create a generative template", getLocalBounds(), juce::Justification::centred);
}

void DaliVisualEditor::resized()
{
    auto r = getLocalBounds();
    header.setBounds(r.removeFromTop(52));
    meters.setBounds(r.removeFromBottom(40));
    tabs.setBounds(r.removeFromRight(juce::jlimit(360, 460, getWidth() / 3)));
    preview.setBounds(r.reduced(8));
    settings.setBounds(getLocalBounds());
}

bool DaliVisualEditor::keyPressed(const juce::KeyPress& k)
{
    if (k.getKeyCode() == 'F' || k.getKeyCode() == 'f') { proc.output.toggle(); return true; }
    if (k == juce::KeyPress::escapeKey)
    {
        if (settings.isVisible()) { settings.setVisible(false); return true; }
        if (proc.output.isOpen()) { proc.output.close(); return true; }
    }
    if (k.getKeyCode() >= '1' && k.getKeyCode() <= '8')
    {
        if (auto* prm = proc.apvts.getParameter(dali::params::id::scene))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(float(k.getKeyCode() - '1')));
            prm->endChangeGesture();
        }
        return true;
    }
    return false;
}

bool DaliVisualEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files) if (dali::ImageProcessor::isSupportedFile(juce::File(f))) return true;
    return false;
}

void DaliVisualEditor::filesDropped(const juce::StringArray& files, int, int)
{
    dragging = false;
    repaint();
    for (auto& path : files)
    {
        const juce::File f(path);
        if (dali::ImageProcessor::isSupportedFile(f) && proc.image.loadFile(f))
        {
            if (auto* prm = proc.apvts.getParameter(dali::params::id::tplEnable))
                if (prm->getValue() < 0.5f) prm->setValueNotifyingHost(1.0f);
            tabs.setCurrentTabIndex(ImageTab);
            return;
        }
    }
}
