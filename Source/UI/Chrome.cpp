#include "Chrome.h"
#include "../Output/OutputManager.h"

namespace dali
{
// =============================================================================
//  shared combos
// =============================================================================
DisplayCombo::DisplayCombo(DaliVisualProcessor& p) : proc(p)
{
    setTooltip("Which display the fullscreen output (GO LIVE) appears on");
    onChange = [this]
    {
        const int idx = getSelectedId() - 1;
        if (idx < 0) return;
        proc.engineState.output.displayIndex = idx;
        if (proc.output.isOpen()) proc.output.open(idx);        // move the live output right away
    };
    refresh();
    startTimer(1500);
}

void DisplayCombo::refresh()
{
    const auto displays = OutputManager::getDisplays();
    lastCount = displays.size();
    clear(juce::dontSendNotification);
    for (auto& d : displays) addItem(d.name, d.index + 1);
    int chosen = proc.engineState.output.displayIndex.load();
    if (chosen < 0 || chosen >= displays.size()) chosen = displays.size() - 1;   // default: last (usually external)
    setSelectedId(chosen + 1, juce::dontSendNotification);
}

void DisplayCombo::timerCallback()
{
    if (isPopupActive()) return;
    const int count = OutputManager::getDisplays().size();
    const int want = proc.engineState.output.displayIndex.load() + 1;
    if (count != lastCount || (want > 0 && want != getSelectedId())) refresh();   // monitor plugged in / changed elsewhere
}

SourceCombo::SourceCombo(DaliVisualProcessor& p) : proc(p)
{
    if (proc.canCaptureSystemAudio()) addItem("System Audio (what you hear)", 2);
    addItem(proc.isStandalone() ? "Audio Input (device)" : "Track Audio (host)", 1);
    setSelectedId(proc.getInputSource() + 1, juce::dontSendNotification);
    setEnabled(proc.isStandalone());
    onChange = [this] { proc.setInputSource(getSelectedId() - 1); };
    startTimer(500);
}

void SourceCombo::timerCallback()
{
    if (!isPopupActive() && getSelectedId() != proc.getInputSource() + 1)
        setSelectedId(proc.getInputSource() + 1, juce::dontSendNotification);
    setTooltip(proc.getInputSourceStatus());
}

// =============================================================================
//  HEADER
// =============================================================================
HeaderBar::HeaderBar(DaliVisualProcessor& p) : proc(p), scene(p, params::id::scene), source(p), display(p)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &scene, &preset, &prev, &next, &presetMenu,
                                                                        &source, &display, &identify, &live, &panelBtn, &settings })
        addAndMakeVisible(c);

    preset.setTextWhenNothingSelected("- unsaved -");
    preset.onChange = [this]
    {
        const int i = preset.getSelectedItemIndex();
        if (i >= 0 && proc.presets.getNames()[i] != proc.presets.getCurrentName()) proc.presets.loadIndex(i);
    };
    prev.onClick = [this] { proc.presets.previous(); };
    next.onClick = [this] { proc.presets.next(); };
    presetMenu.onClick = [this] { showPresetMenu(); };
    identify.onClick = [this] { proc.output.identifyDisplays(); };
    live.onClick = [this] { proc.output.toggle(); updateLiveButton(); };
    panelBtn.setClickingTogglesState(true);
    panelBtn.setToggleState(true, juce::dontSendNotification);
    panelBtn.onClick = [this] { if (onTogglePanel) onTogglePanel(); };
    settings.onClick = [this] { if (onSettings) onSettings(); };

    prev.setTooltip("Previous preset");
    next.setTooltip("Next preset");
    presetMenu.setTooltip("Save, save as, duplicate, delete ...");
    identify.setTooltip("Show the number of every display on its screen");
    live.setTooltip("Fullscreen output on the chosen display (key F). ESC or double-click ends it.");
    panelBtn.setTooltip("Show / hide the side panel (Tab) - bigger preview for performing");
    scene.setTooltip("Scene (keys 1-8)");

    proc.presets.addChangeListener(this);
    proc.output.addChangeListener(this);
    refreshPresets();
    updateLiveButton();
    startTimerHz(4);
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

void HeaderBar::updateLiveButton()
{
    const bool on = proc.output.isOpen();
    live.setToggleState(on, juce::dontSendNotification);
    live.setButtonText(on ? "LIVE  - STOP" : "GO LIVE");
    live.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe0304a));
}

void HeaderBar::askName(const juce::String& title, const juce::String& initial, std::function<void(juce::String)> done)
{
    auto* w = new juce::AlertWindow(title, "Preset name:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor("name", initial);
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->setAlwaysOnTop(true);
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
                juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                                 .withIconType(juce::MessageBoxIconType::QuestionIcon)
                                                 .withTitle("Delete preset")
                                                 .withMessage("Delete \"" + current + "\"?")
                                                 .withButton("Delete")
                                                 .withButton("Cancel"),
                                             [this, current](int result) { if (result == 1) proc.presets.remove(current); });
                break;
            case 5: pm.refresh(); break;
            case 6: PresetManager::getPresetFolder().revealToUser(); break;
            default: break;
        }
    });
}

void HeaderBar::paint(juce::Graphics& g)
{
    g.fillAll(colours::bg);
    g.setColour(colours::outline);
    g.fillRect(getLocalBounds().removeFromBottom(1));

    juce::ColourGradient grad(colours::accent, 14.0f, 0.0f, colours::accent2, 180.0f, 0.0f, false);
    g.setGradientFill(grad);
    g.setFont(juce::Font(juce::FontOptions(21.0f, juce::Font::bold)).withExtraKerningFactor(0.12f));
    g.drawText("DALI VISUAL", juce::Rectangle<int>(14, 8, 180, 26), juce::Justification::centredLeft);
    g.setColour(colours::textDim);
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)).withExtraKerningFactor(0.3f));
    g.drawText("BY DALI AUDIO", juce::Rectangle<int>(15, 34, 180, 12), juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)).withExtraKerningFactor(0.15f));
    for (auto& c : captions)
    {
        g.setColour(colours::textDim);
        g.drawText(c.text, c.area, juce::Justification::centredLeft);
    }
    // separators between groups
    g.setColour(colours::outline);
    for (auto* c : { (juce::Component*) &source, (juce::Component*) &display, (juce::Component*) &panelBtn })
        g.fillRect(c->getX() - 9, 10, 1, getHeight() - 20);
}

void HeaderBar::resized()
{
    captions.clearQuick();
    auto r = getLocalBounds().reduced(10, 0);
    r.removeFromLeft(185);
    const int capH = 13, ctlY = 20, ctlH = 26;
    auto place = [&](juce::Component& c, int x, int w, const juce::String& caption = {})
    {
        c.setBounds(x, ctlY, w, ctlH);
        if (caption.isNotEmpty()) captions.add({ caption, juce::Rectangle<int>(x + 2, 5, w, capH) });
    };

    // right side (fixed)
    int x = r.getRight();
    x -= 86;  place(settings, x, 86);
    x -= 70;  place(panelBtn, x, 64);
    x -= 18 + 104; place(live, x, 104);
    x -= 40;  place(identify, x, 36, "");
    x -= 200; place(display, x, 196, "OUTPUT DISPLAY");
    const int sourceW = 188;
    x -= 18 + sourceW; place(source, x, sourceW, "AUDIO SOURCE");
    const int rightStart = x - 18;

    // left side (flexible): scene + preset
    int lx = r.getX();
    const int avail = rightStart - lx;
    const int sceneW = juce::jlimit(150, 220, avail * 2 / 5);
    place(scene, lx, sceneW, "SCENE");
    lx += sceneW + 12;
    const int presetW = juce::jmax(120, rightStart - lx - 3 * 30 - 6);
    place(preset, lx, presetW, "PRESET");
    lx += presetW + 4;
    place(prev, lx, 28);       lx += 30;
    place(next, lx, 28);       lx += 30;
    place(presetMenu, lx, 30);
}

// =============================================================================
//  METERS
// =============================================================================
void MeterBar::timerCallback()
{
    const auto f = proc.analyzer.snapshot().features;
    const auto& t = proc.engineState.telemetry;
    activity = t.activity.load();
    auto fall = [](float cur, float v) { return v > cur ? v : cur * 0.86f + v * 0.14f; };
    const float a = activity;
    bass = fall(bass, f.bassEnv * a); mid = fall(mid, f.midEnv * a); high = fall(high, f.highEnv * a);
    energy = fall(energy, f.energy * a);
    kick = f.kick * a; snare = f.snare * a; hat = f.hat * a; build = f.build * a; drop = f.drop * a;
    bpm = t.bpm.load(); fps = t.previewFps.load(); outFps = t.outputFps.load();
    cpu = t.cpuLoad.load(); frameMs = t.frameMs.load(); source = t.clockSource.load();
    output = t.outputActive.load();
    repaint();
}

void MeterBar::paint(juce::Graphics& g)
{
    using namespace colours;
    g.fillAll(bg);
    g.setColour(outline);
    g.fillRect(getLocalBounds().removeFromTop(1));

    auto r = getLocalBounds().reduced(14, 8);
    const juce::Font labelFont(juce::FontOptions(9.5f, juce::Font::bold));
    const juce::Font valueFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));

    auto bar = [&](const char* name, float v, juce::Colour c, int w)
    {
        auto cell = r.removeFromLeft(w);
        r.removeFromLeft(10);
        g.setColour(textDim); g.setFont(labelFont);
        g.drawText(name, cell.removeFromLeft(40), juce::Justification::centredLeft);
        auto b = cell.withSizeKeepingCentre(cell.getWidth(), 7).toFloat();
        g.setColour(panel2); g.fillRoundedRectangle(b, 3.5f);
        g.setColour(c); g.fillRoundedRectangle(b.withWidth(b.getWidth() * juce::jlimit(0.0f, 1.0f, v)), 3.5f);
    };
    auto led = [&](const char* name, float v, juce::Colour c)
    {
        auto cell = r.removeFromLeft(54);
        g.setColour(textDim); g.setFont(labelFont);
        g.drawText(name, cell.removeFromLeft(36), juce::Justification::centredLeft);
        auto d = cell.withSizeKeepingCentre(12, 12).toFloat();
        g.setColour(panel2); g.fillEllipse(d);
        g.setColour(c.withAlpha(juce::jlimit(0.0f, 1.0f, v))); g.fillEllipse(d);
    };
    auto value = [&](const char* name, const juce::String& text, int w)
    {
        auto cell = r.removeFromLeft(w);
        g.setColour(textDim); g.setFont(labelFont);
        g.drawText(name, cell.removeFromLeft(30), juce::Justification::centredLeft);
        g.setColour(colours::text); g.setFont(valueFont);
        g.drawText(text, cell, juce::Justification::centredLeft);
    };

    bar("BASS", bass, accent, 112);
    bar("MID", mid, accent2, 112);
    bar("HIGH", high, modRing, 112);
    bar("ENERGY", energy, colours::text.withAlpha(0.8f), 112);
    led("KICK", kick, accent);
    led("SNARE", snare, accent2);
    led("HAT", hat, modRing);
    r.removeFromLeft(6);
    bar("BUILD", juce::jmax(build, drop), drop > 0.05f ? juce::Colour(0xffff5c7a) : learn, 110);

    static const char* srcNames[] = { "HOST", "DETECT", "INT" };
    value("BPM", activity > 0.05f ? juce::String(bpm, 1) + " " + srcNames[juce::jlimit(0, 2, source)] : juce::String("--"), 118);
    value("FPS", output ? juce::String(juce::roundToInt(outFps)) + " LIVE" : juce::String(juce::roundToInt(fps)), 84);
    value("CPU", juce::String(cpu * 100.0f, 1) + "%", 76);

    if (activity < 0.05f)
    {
        g.setColour(learn);
        g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        const juce::String hint = proc.getInputSource() == DaliVisualProcessor::SystemAudio
                                      ? "NO SIGNAL - play something on this computer"
                                      : (proc.isStandalone() ? "NO SIGNAL - choose an input or System Audio"
                                                             : "NO SIGNAL - play the track");
        g.drawFittedText(hint, r, juce::Justification::centredRight, 1);
    }
}

// =============================================================================
//  SETTINGS
// =============================================================================
SettingsPanel::SettingsPanel(DaliVisualProcessor& p) : proc(p), display(p), source(p)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &outHeader, &audioHeader, &midiHeader, &infoHeader, &display, &source, &resolution, &displayLabel,
             &resolutionLabel, &sourceLabel, &sourceStatus, &midiLast, &info, &vsync, &previewWhileOutput,
             &noteScenes, &programScenes, &identify, &openOutput, &clearMidi, &factory })
        addAndMakeVisible(c);

    displayLabel.setText("Output display", juce::dontSendNotification);
    resolutionLabel.setText("Render resolution", juce::dontSendNotification);
    sourceLabel.setText("Listen to", juce::dontSendNotification);
    for (auto* l : { &displayLabel, &resolutionLabel, &sourceLabel, &sourceStatus, &midiLast, &info })
    {
        l->setFont(juce::Font(juce::FontOptions(12.5f)));
        l->setColour(juce::Label::textColourId, colours::textDim);
    }
    info.setJustificationType(juce::Justification::topLeft);
    sourceStatus.setJustificationType(juce::Justification::topLeft);

    resolution.addItem("50 %  (fastest)", 1);
    resolution.addItem("75 %", 2);
    resolution.addItem("100 %  (native)", 3);
    resolution.onChange = [this] { proc.engineState.output.renderScaleIndex = resolution.getSelectedId() - 1; };
    vsync.onClick = [this] { proc.engineState.output.vsync = vsync.getToggleState(); };
    previewWhileOutput.onClick = [this] { proc.engineState.output.previewWhileOutput = previewWhileOutput.getToggleState(); };
    noteScenes.onClick = [this] { proc.midi.noteSceneSwitching = noteScenes.getToggleState(); };
    programScenes.onClick = [this] { proc.midi.programChangeScenes = programScenes.getToggleState(); };
    identify.onClick = [this] { proc.output.identifyDisplays(); };
    openOutput.onClick = [this] { proc.output.toggle(); refresh(); };
    clearMidi.onClick = [this] { proc.midi.clearAll(); };
    factory.onClick = [this] { proc.presets.installFactoryPresets(true); };
    setSize(580, 640);
    refresh();
    startTimerHz(4);
}

void SettingsPanel::refresh()
{
    display.refresh();
    resolution.setSelectedId(proc.engineState.output.renderScaleIndex.load() + 1, juce::dontSendNotification);
    vsync.setToggleState(proc.engineState.output.vsync.load(), juce::dontSendNotification);
    previewWhileOutput.setToggleState(proc.engineState.output.previewWhileOutput.load(), juce::dontSendNotification);
    noteScenes.setToggleState(proc.midi.noteSceneSwitching.load(), juce::dontSendNotification);
    programScenes.setToggleState(proc.midi.programChangeScenes.load(), juce::dontSendNotification);
    openOutput.setButtonText(proc.output.isOpen() ? "STOP LIVE OUTPUT" : "GO LIVE");
}

void SettingsPanel::timerCallback()
{
    if (!isShowing()) return;
    openOutput.setButtonText(proc.output.isOpen() ? "STOP LIVE OUTPUT" : "GO LIVE");
    sourceStatus.setText(proc.getInputSourceStatus(), juce::dontSendNotification);
    const auto last = proc.midi.getLastMessageText();
    midiLast.setText("Last MIDI: " + (last.isNotEmpty() ? last : juce::String("-")), juce::dontSendNotification);
    juce::String renderer;
    { const juce::SpinLock::ScopedLockType sl(proc.engineState.telemetry.infoLock); renderer = proc.engineState.telemetry.rendererInfo; }
    juce::String s;
    s << "Renderer: " << (renderer.isNotEmpty() ? renderer : juce::String("starting...")) << "\n"
      << "Presets: " << PresetManager::getPresetFolder().getFullPathName() << "\n"
      << (proc.isStandalone() ? "Standalone - with 'Audio Input', choose the device under Options > Audio/MIDI Settings."
                              : "Plug-in - listens to the track it is inserted on; audio passes through unchanged.");
    info.setText(s, juce::dontSendNotification);
}

void SettingsPanel::paint(juce::Graphics& g) { g.fillAll(colours::panel); }

void SettingsPanel::resized()
{
    auto r = getLocalBounds().reduced(22, 16);
    auto row = [&](int h) { auto x = r.removeFromTop(h); return x; };

    outHeader.setBounds(row(24));
    auto a = row(30); displayLabel.setBounds(a.removeFromLeft(150)); identify.setBounds(a.removeFromRight(140).reduced(0, 2));
    a.removeFromRight(8); display.setBounds(a.reduced(0, 2));
    a = row(30); resolutionLabel.setBounds(a.removeFromLeft(150)); resolution.setBounds(a.reduced(0, 2));
    vsync.setBounds(row(26));
    previewWhileOutput.setBounds(row(26));
    openOutput.setBounds(row(34).withWidth(200).reduced(0, 3));
    r.removeFromTop(10);

    audioHeader.setBounds(row(24));
    a = row(30); sourceLabel.setBounds(a.removeFromLeft(150)); source.setBounds(a.reduced(0, 2));
    sourceStatus.setBounds(row(34).withTrimmedLeft(150));
    r.removeFromTop(8);

    midiHeader.setBounds(row(24));
    noteScenes.setBounds(row(26));
    programScenes.setBounds(row(26));
    midiLast.setBounds(row(22));
    a = row(32);
    clearMidi.setBounds(a.removeFromLeft(230).reduced(0, 2));
    a.removeFromLeft(10);
    factory.setBounds(a.removeFromLeft(230).reduced(0, 2));
    r.removeFromTop(10);

    infoHeader.setBounds(row(24));
    info.setBounds(r);
}

SettingsWindow::SettingsWindow(DaliVisualProcessor& p, juce::LookAndFeel& lnf, juce::Component* centreOn)
    : juce::DocumentWindow("Dali Visual - Settings", colours::panel, juce::DocumentWindow::closeButton, true)
{
    setLookAndFeel(&lnf);
    setUsingNativeTitleBar(true);
    setContentOwned(new SettingsPanel(p), true);
    setResizable(false, false);
    show(centreOn);
}

void SettingsWindow::show(juce::Component* centreOn)
{
    if (auto* panel = dynamic_cast<SettingsPanel*>(getContentComponent())) panel->refresh();
    if (centreOn != nullptr) centreAroundComponent(centreOn, getWidth(), getHeight());
    setAlwaysOnTop(true);
    setVisible(true);
    toFront(true);
}
} // namespace dali
