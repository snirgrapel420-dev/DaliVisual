#include "Chrome.h"
#include "../Output/OutputManager.h"

namespace dali
{
// =============================================================================
//  HEADER
// =============================================================================
HeaderBar::HeaderBar(DaliVisualProcessor& p) : proc(p), scene(p, params::id::scene)
{
    for (auto* c : { (juce::Component*) &scene, &preset, &prev, &next, &presetMenu, &fullscreen, &displayMenu, &settings,
                     &sceneLabel, &presetLabel })
        addAndMakeVisible(c);
    for (auto* l : { &sceneLabel, &presetLabel })
    {
        l->setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        l->setColour(juce::Label::textColourId, colours::textDim);
    }
    sceneLabel.setText("SCENE", juce::dontSendNotification);
    presetLabel.setText("PRESET", juce::dontSendNotification);

    preset.setTextWhenNothingSelected("— unsaved —");
    preset.onChange = [this]
    {
        const int i = preset.getSelectedItemIndex();
        if (i >= 0 && proc.presets.getNames()[i] != proc.presets.getCurrentName()) proc.presets.loadIndex(i);
    };
    prev.onClick = [this] { proc.presets.previous(); };
    next.onClick = [this] { proc.presets.next(); };
    presetMenu.onClick = [this] { showPresetMenu(); };
    fullscreen.onClick = [this] { proc.output.toggle(); };
    displayMenu.onClick = [this] { showFullscreenMenu(); };
    settings.onClick = [this] { if (onSettings) onSettings(); };
    fullscreen.setTooltip("Fullscreen output on the selected display (F).  ESC closes it.");
    displayMenu.setTooltip("Choose the output display");

    proc.presets.addChangeListener(this);
    proc.output.addChangeListener(this);
    refreshPresets();
    updateFullscreenButton();
}

HeaderBar::~HeaderBar()
{
    proc.presets.removeChangeListener(this);
    proc.output.removeChangeListener(this);
}

void HeaderBar::refreshPresets()
{
    preset.clear(juce::dontSendNotification);
    const auto names = proc.presets.getNames();
    for (int i = 0; i < names.size(); ++i) preset.addItem(names[i], i + 1);
    const int cur = proc.presets.getCurrentIndex();
    if (cur >= 0) preset.setSelectedItemIndex(cur, juce::dontSendNotification);
}

void HeaderBar::updateFullscreenButton()
{
    fullscreen.setToggleState(proc.output.isOpen(), juce::dontSendNotification);
    fullscreen.setButtonText(proc.output.isOpen() ? "OUTPUT LIVE" : "FULLSCREEN");
}

void HeaderBar::askName(const juce::String& title, const juce::String& initial, std::function<void(juce::String)> done)
{
    auto* w = new juce::AlertWindow(title, "Preset name:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor("name", initial);
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->enterModalState(true, juce::ModalCallbackFunction::create([w, done](int r)
    {
        if (r == 1)
        {
            const auto n = w->getTextEditorContents("name").trim();
            if (n.isNotEmpty()) done(n);
        }
    }), true);
}

void HeaderBar::showPresetMenu()
{
    const auto current = proc.presets.getCurrentName();
    juce::PopupMenu m;
    m.addItem(1, "Save" + (current.isNotEmpty() ? " \"" + current + "\"" : juce::String()), current.isNotEmpty());
    m.addItem(2, "Save As...");
    m.addItem(3, "Duplicate");
    m.addItem(4, "Delete" + (current.isNotEmpty() ? " \"" + current + "\"" : juce::String()), current.isNotEmpty());
    m.addSeparator();
    m.addItem(5, "Refresh list");
    m.addItem(6, "Show preset folder");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetMenu), [this, current](int r)
    {
        auto& pm = proc.presets;
        switch (r)
        {
            case 1: pm.save(current); break;
            case 2: askName("Save Preset As", current.isNotEmpty() ? current : "My Preset", [this](juce::String n) { proc.presets.save(n); }); break;
            case 3: pm.duplicate(); break;
            case 4:
                juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Delete preset",
                    "Delete \"" + current + "\"?", "Delete", "Cancel", nullptr,
                    juce::ModalCallbackFunction::create([this, current](int ok) { if (ok) proc.presets.remove(current); }));
                break;
            case 5: pm.refresh(); break;
            case 6: PresetManager::getPresetFolder().revealToUser(); break;
            default: break;
        }
    });
}

void HeaderBar::showFullscreenMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader("Output display");
    const auto displays = OutputManager::getDisplays();
    const int chosen = proc.engineState.output.displayIndex.load();
    for (auto& d : displays) m.addItem(100 + d.index, d.name, true, d.index == chosen || (chosen < 0 && d.index == displays.size() - 1));
    m.addSeparator();
    m.addItem(1, proc.output.isOpen() ? "Close output" : "Open output");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&displayMenu), [this](int r)
    {
        if (r >= 100) { proc.engineState.output.displayIndex = r - 100; if (proc.output.isOpen()) proc.output.open(r - 100); }
        else if (r == 1) proc.output.toggle();
    });
}

void HeaderBar::paint(juce::Graphics& g)
{
    g.fillAll(colours::bg);
    g.setColour(colours::outline);
    g.fillRect(getLocalBounds().removeFromBottom(1));

    auto r = getLocalBounds().reduced(14, 0);
    juce::ColourGradient grad(colours::accent, float(r.getX()), 0.0f, colours::accent2, float(r.getX() + 180), 0.0f, false);
    g.setGradientFill(grad);
    g.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)).withExtraKerningFactor(0.12f));
    g.drawText("DALI VISUAL", r.removeFromLeft(190).withTrimmedBottom(12), juce::Justification::bottomLeft);
    g.setColour(colours::textDim);
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)).withExtraKerningFactor(0.3f));
    g.drawText("BY DALI AUDIO", juce::Rectangle<int>(14, getHeight() - 16, 190, 12), juce::Justification::left);
}

void HeaderBar::resized()
{
    auto r = getLocalBounds().reduced(10, 8);
    r.removeFromLeft(200);
    settings.setBounds(r.removeFromRight(90).reduced(2, 4));
    r.removeFromRight(6);
    displayMenu.setBounds(r.removeFromRight(26).reduced(1, 4));
    fullscreen.setBounds(r.removeFromRight(116).reduced(1, 4));
    r.removeFromRight(14);

    auto sceneArea = r.removeFromLeft(juce::jmin(260, r.getWidth() / 3));
    sceneLabel.setBounds(sceneArea.removeFromLeft(48));
    scene.setBounds(sceneArea.reduced(2, 5));
    r.removeFromLeft(14);
    presetLabel.setBounds(r.removeFromLeft(54));
    presetMenu.setBounds(r.removeFromRight(72).reduced(2, 5));
    next.setBounds(r.removeFromRight(28).reduced(1, 5));
    prev.setBounds(r.removeFromRight(28).reduced(1, 5));
    preset.setBounds(r.reduced(2, 5));
}

// =============================================================================
//  METERS
// =============================================================================
void MeterBar::timerCallback()
{
    const auto f = proc.analyzer.snapshot().features;
    auto fall = [](float cur, float v) { return v > cur ? v : cur * 0.88f + v * 0.12f; };
    bass = fall(bass, f.bassEnv); mid = fall(mid, f.midEnv); high = fall(high, f.highEnv); energy = fall(energy, f.energy);
    const auto& t = proc.engineState.telemetry;
    beat = t.beatPulse.load();
    bpm = t.bpm.load(); fps = t.previewFps.load(); outFps = t.outputFps.load();
    cpu = t.cpuLoad.load(); frameMs = t.frameMs.load(); source = t.clockSource.load();
    output = t.outputActive.load(); silent = f.silent;
    repaint();
}

void MeterBar::paint(juce::Graphics& g)
{
    using namespace colours;
    g.fillAll(bg);
    g.setColour(outline);
    g.fillRect(getLocalBounds().removeFromTop(1));

    auto r = getLocalBounds().reduced(14, 9);
    auto meter = [&](const char* name, float v, juce::Colour c)
    {
        auto cell = r.removeFromLeft(128);
        r.removeFromLeft(10);
        g.setColour(textDim);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(name, cell.removeFromLeft(46), juce::Justification::centredLeft);
        auto bar = cell.withSizeKeepingCentre(cell.getWidth(), 8).toFloat();
        g.setColour(panel2);
        g.fillRoundedRectangle(bar, 4.0f);
        g.setColour(c);
        g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * juce::jlimit(0.0f, 1.0f, v)), 4.0f);
    };
    meter("BASS", bass, accent);
    meter("MID", mid, accent2);
    meter("HIGH", high, modRing);
    meter("ENERGY", energy, text.withAlpha(0.8f));

    auto beatCell = r.removeFromLeft(70);
    g.setColour(textDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("BEAT", beatCell.removeFromLeft(40), juce::Justification::centredLeft);
    auto led = beatCell.withSizeKeepingCentre(14, 14).toFloat();
    g.setColour(panel2);
    g.fillEllipse(led);
    g.setColour(accent.withAlpha(juce::jlimit(0.0f, 1.0f, beat)));
    g.fillEllipse(led);

    static const char* srcNames[] = { "HOST", "DETECT", "INT" };
    auto text = [&](const juce::String& label, const juce::String& value, int w)
    {
        auto cell = r.removeFromLeft(w);
        g.setColour(textDim);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(label, cell.removeFromLeft(32), juce::Justification::centredLeft);
        g.setColour(colours::text);
        g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.5f, juce::Font::plain)));
        g.drawText(value, cell, juce::Justification::centredLeft);
    };
    text("BPM", juce::String(bpm, 1) + " " + srcNames[juce::jlimit(0, 2, source)], 130);
    text("FPS", output ? juce::String(juce::roundToInt(outFps)) + " OUT / " + juce::String(juce::roundToInt(fps))
                       : juce::String(juce::roundToInt(fps)) + "  " + juce::String(frameMs, 1) + "ms", 150);
    text("CPU", juce::String(cpu * 100.0f, 1) + "% audio", 120);
    if (silent)
    {
        g.setColour(learn);
        g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        g.drawText("NO INPUT", r, juce::Justification::centredRight);
    }
}

// =============================================================================
//  SETTINGS
// =============================================================================
SettingsPanel::SettingsPanel(DaliVisualProcessor& p) : proc(p)
{
    for (auto* c : { (juce::Component*) &outHeader, &midiHeader, &infoHeader, &display, &resolution, &displayLabel,
                     &resolutionLabel, &midiLast, &info, &vsync, &previewWhileOutput, &noteScenes, &programScenes,
                     &openOutput, &clearMidi, &factory, &close })
        addAndMakeVisible(c);
    displayLabel.setText("Output display", juce::dontSendNotification);
    resolutionLabel.setText("Render resolution", juce::dontSendNotification);
    for (auto* l : { &displayLabel, &resolutionLabel, &midiLast, &info })
    {
        l->setFont(juce::Font(juce::FontOptions(12.0f)));
        l->setColour(juce::Label::textColourId, colours::textDim);
    }
    info.setJustificationType(juce::Justification::topLeft);

    resolution.addItem("50 %  (fastest)", 1);
    resolution.addItem("75 %", 2);
    resolution.addItem("100 %  (native)", 3);
    resolution.onChange = [this] { proc.engineState.output.renderScaleIndex = resolution.getSelectedId() - 1; };
    display.onChange = [this]
    {
        proc.engineState.output.displayIndex = display.getSelectedId() - 1;
        if (proc.output.isOpen()) proc.output.open(display.getSelectedId() - 1);
    };
    vsync.onClick = [this] { proc.engineState.output.vsync = vsync.getToggleState(); };
    previewWhileOutput.onClick = [this] { proc.engineState.output.previewWhileOutput = previewWhileOutput.getToggleState(); };
    noteScenes.onClick = [this] { proc.midi.noteSceneSwitching = noteScenes.getToggleState(); };
    programScenes.onClick = [this] { proc.midi.programChangeScenes = programScenes.getToggleState(); };
    openOutput.onClick = [this] { proc.output.toggle(); refresh(); };
    clearMidi.onClick = [this] { proc.midi.clearAll(); };
    factory.onClick = [this] { proc.presets.installFactoryPresets(true); };
    close.onClick = [this] { if (onClose) onClose(); };
    startTimerHz(4);
}

void SettingsPanel::refresh()
{
    display.clear(juce::dontSendNotification);
    const auto displays = OutputManager::getDisplays();
    for (auto& d : displays) display.addItem(d.name, d.index + 1);
    int chosen = proc.engineState.output.displayIndex.load();
    if (chosen < 0 || chosen >= displays.size()) chosen = displays.size() - 1;
    display.setSelectedId(chosen + 1, juce::dontSendNotification);
    resolution.setSelectedId(proc.engineState.output.renderScaleIndex.load() + 1, juce::dontSendNotification);
    vsync.setToggleState(proc.engineState.output.vsync.load(), juce::dontSendNotification);
    previewWhileOutput.setToggleState(proc.engineState.output.previewWhileOutput.load(), juce::dontSendNotification);
    noteScenes.setToggleState(proc.midi.noteSceneSwitching.load(), juce::dontSendNotification);
    programScenes.setToggleState(proc.midi.programChangeScenes.load(), juce::dontSendNotification);
    openOutput.setButtonText(proc.output.isOpen() ? "Close Fullscreen Output" : "Open Fullscreen Output");
}

void SettingsPanel::timerCallback()
{
    if (!isVisible()) return;
    const auto last = proc.midi.getLastMessageText();
    midiLast.setText("Last MIDI: " + (last.isNotEmpty() ? last : juce::String("—")), juce::dontSendNotification);
    juce::String renderer;
    { const juce::SpinLock::ScopedLockType sl(proc.engineState.telemetry.infoLock); renderer = proc.engineState.telemetry.rendererInfo; }
    juce::String s;
    s << "Renderer: " << (renderer.isNotEmpty() ? renderer : juce::String("starting...")) << "\n"
      << "Presets: " << PresetManager::getPresetFolder().getFullPathName() << "\n"
      << "Mode: " << (proc.isStandalone() ? "Standalone - choose the input under Options > Audio/MIDI Settings "
                                             "(un-mute the input there if JUCE muted it)."
                                          : "Plug-in - audio passes through unchanged.");
    info.setText(s, juce::dontSendNotification);
}

void SettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(colours::bg.withAlpha(0.75f));
    auto card = getLocalBounds().withSizeKeepingCentre(juce::jmin(560, getWidth() - 40), juce::jmin(560, getHeight() - 40)).toFloat();
    g.setColour(colours::panel);
    g.fillRoundedRectangle(card, 10.0f);
    g.setColour(colours::accent.withAlpha(0.6f));
    g.drawRoundedRectangle(card, 10.0f, 1.0f);
}

void SettingsPanel::resized()
{
    auto r = getLocalBounds().withSizeKeepingCentre(juce::jmin(560, getWidth() - 40), juce::jmin(560, getHeight() - 40)).reduced(20);
    close.setBounds(r.getRight() - 80, r.getY(), 80, 26);
    outHeader.setBounds(r.removeFromTop(24));
    auto row = r.removeFromTop(28);
    displayLabel.setBounds(row.removeFromLeft(150)); display.setBounds(row.reduced(0, 2));
    row = r.removeFromTop(28);
    resolutionLabel.setBounds(row.removeFromLeft(150)); resolution.setBounds(row.reduced(0, 2));
    vsync.setBounds(r.removeFromTop(26));
    previewWhileOutput.setBounds(r.removeFromTop(26));
    openOutput.setBounds(r.removeFromTop(30).withWidth(240).reduced(0, 2));
    r.removeFromTop(10);
    midiHeader.setBounds(r.removeFromTop(24));
    noteScenes.setBounds(r.removeFromTop(26));
    programScenes.setBounds(r.removeFromTop(26));
    midiLast.setBounds(r.removeFromTop(22));
    row = r.removeFromTop(30);
    clearMidi.setBounds(row.removeFromLeft(220).reduced(0, 2));
    row.removeFromLeft(10);
    factory.setBounds(row.removeFromLeft(220).reduced(0, 2));
    r.removeFromTop(10);
    infoHeader.setBounds(r.removeFromTop(24));
    info.setBounds(r);
}
} // namespace dali
