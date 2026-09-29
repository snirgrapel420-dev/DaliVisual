#include "Panels.h"
#include "../Render/Library.h"
#include "../Render/Palettes.h"

namespace dali
{
namespace
{
constexpr int kPad = 10, kKnobH = 64, kHeaderH = 22;

juce::Label& styleSmall(juce::Label& l, bool dim = true)
{
    l.setFont(juce::Font(juce::FontOptions(11.5f)));
    l.setColour(juce::Label::textColourId, dim ? colours::textDim : colours::text);
    return l;
}
}

// =============================================================================
//  SCENE
// =============================================================================
ScenePanel::ScenePanel(DaliVisualProcessor& p)
    : proc(p),
      macroA(p, params::id::macroA), macroB(p, params::id::macroB), macroC(p, params::id::macroC), macroD(p, params::id::macroD),
      intensity(p, params::id::intensity, "Intensity"), speed(p, params::id::speed, "Motion")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &sceneHeader, &macroHeader, &globalHeader, &description,
                     &macroA, &macroB, &macroC, &macroD, &intensity, &speed })
        addAndMakeVisible(c);

    int i = 0;
    for (auto& s : sceneLibrary())
    {
        auto* b = sceneButtons.add(new juce::TextButton(s.name));
        b->setClickingTogglesState(false);
        b->onClick = [this, i]
        {
            if (auto* prm = proc.apvts.getParameter(params::id::scene))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost(prm->convertTo0to1(float(i)));
                prm->endChangeGesture();
            }
        };
        addAndMakeVisible(b);
        ++i;
    }
    styleSmall(description);
    description.setJustificationType(juce::Justification::topLeft);
    proc.apvts.addParameterListener(params::id::scene, this);
    handleAsyncUpdate();
}

ScenePanel::~ScenePanel() { proc.apvts.removeParameterListener(params::id::scene, this); }

void ScenePanel::handleAsyncUpdate()
{
    const int s = juce::roundToInt(proc.apvts.getRawParameterValue(params::id::scene)->load());
    for (int i = 0; i < sceneButtons.size(); ++i) sceneButtons[i]->setToggleState(i == s, juce::dontSendNotification);
    const auto& info = sceneLibrary()[size_t(juce::jlimit(0, int(sceneLibrary().size()) - 1, s))];
    macroA.setLabel(info.macro[0]); macroB.setLabel(info.macro[1]);
    macroC.setLabel(info.macro[2]); macroD.setLabel(info.macro[3]);
    description.setText(info.description, juce::dontSendNotification);
}

int ScenePanel::preferredHeight(int) { return kPad + kHeaderH + 4 * 34 + 44 + kHeaderH + kKnobH * 2 + kHeaderH + kKnobH + kPad * 4; }

void ScenePanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    sceneHeader.setBounds(r.removeFromTop(kHeaderH));
    auto grid = r.removeFromTop(4 * 34);
    const int bw = grid.getWidth() / 2;
    for (int i = 0; i < sceneButtons.size(); ++i)
        sceneButtons[i]->setBounds(grid.getX() + (i % 2) * bw + 2, grid.getY() + (i / 2) * 34 + 2, bw - 4, 30);
    description.setBounds(r.removeFromTop(44));
    r.removeFromTop(kPad);
    macroHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &macroA, &macroB, &macroC, &macroD }, r.removeFromTop(kKnobH * 2), 2, kKnobH);
    r.removeFromTop(kPad);
    globalHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &intensity, &speed }, r.removeFromTop(kKnobH), 2, kKnobH);
}

// =============================================================================
//  AUDIO
// =============================================================================
AudioPanel::AudioPanel(DaliVisualProcessor& p)
    : proc(p),
      sensitivity(p, params::id::sensitivity, "Sensitivity"), smoothing(p, params::id::smoothing, "Smoothing"),
      bass(p, params::id::reactBass, "Bass"), mid(p, params::id::reactMid, "Mid"), high(p, params::id::reactHigh, "High"),
      transient(p, params::id::reactTransient, "Transient"), internalBpm(p, params::id::internalBpm, "Internal BPM"),
      syncSource(p, params::id::syncSource), syncDiv(p, params::id::syncDiv)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &inputHeader, &reactHeader, &syncHeader, &sensitivity, &smoothing, &bass, &mid,
                     &high, &transient, &internalBpm, &syncSource, &syncDiv, &syncSourceLabel, &syncDivLabel, &readout })
        addAndMakeVisible(c);
    styleSmall(syncSourceLabel).setText("Clock source", juce::dontSendNotification);
    styleSmall(syncDivLabel).setText("Sync division (LFO sources)", juce::dontSendNotification);
    styleSmall(readout, false);
    readout.setJustificationType(juce::Justification::topLeft);
    readout.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)));
    startTimerHz(10);
}

void AudioPanel::timerCallback()
{
    const auto f = proc.analyzer.snapshot().features;
    static const char* src[] = { "HOST", "DETECTED", "INTERNAL" };
    const auto& t = proc.engineState.telemetry;
    juce::String s;
    s << "Clock      " << juce::String(t.bpm.load(), 1) << " BPM  (" << src[juce::jlimit(0, 2, t.clockSource.load())] << ")\n"
      << "Detected   " << (f.bpm > 0 ? juce::String(f.bpm, 1) + " BPM" : juce::String("-"))
      << "   conf " << juce::String(juce::roundToInt(f.bpmConfidence * 100.0f)) << "%\n"
      << "Centroid   " << juce::String(f.centroid, 2) << "    Flux " << juce::String(f.flux, 2) << "\n"
      << "Width      " << juce::String(f.width, 2) << "    Pan  " << juce::String(f.pan, 2)
      << (f.silent ? "\nInput      silent" : "");
    readout.setText(s, juce::dontSendNotification);
}

int AudioPanel::preferredHeight(int) { return kPad * 5 + kHeaderH * 3 + kKnobH * 3 + 50 + 50 + 90; }

void AudioPanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    inputHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &sensitivity, &smoothing }, r.removeFromTop(kKnobH), 3, kKnobH);
    readout.setBounds(r.removeFromTop(90));
    r.removeFromTop(kPad);
    reactHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &bass, &mid, &high, &transient }, r.removeFromTop(kKnobH), 4, kKnobH);
    r.removeFromTop(kPad);
    syncHeader.setBounds(r.removeFromTop(kHeaderH));
    auto row = r.removeFromTop(50);
    syncSourceLabel.setBounds(row.removeFromTop(18));
    syncSource.setBounds(row.removeFromTop(26).withWidth(r.getWidth()));
    row = r.removeFromTop(50);
    syncDivLabel.setBounds(row.removeFromTop(18));
    syncDiv.setBounds(row.removeFromTop(26).withWidth(r.getWidth()));
    layoutKnobGrid({ &internalBpm }, r.removeFromTop(kKnobH), 3, kKnobH);
}

// =============================================================================
//  MOD — one row per slot
// =============================================================================
class ModPanel::Row : public juce::Component
{
public:
    Row(DaliVisualProcessor& p, int slotIndex) : proc(p), index(slotIndex)
    {
        number.setText(juce::String(slotIndex + 1).paddedLeft('0', 2), juce::dontSendNotification);
        styleSmall(number);
        addAndMakeVisible(number);
        enabled.setTooltip("Enable slot");
        addAndMakeVisible(enabled);

        const auto names = ModulationSource::allNames();
        for (int i = 0; i < names.size(); ++i) source.addItem(names[i], i + 1);
        target.addItem("- target -", 1);
        juce::String lastGroup;
        for (int t = 0; t < ModulationTarget::count(); ++t)
        {
            const auto& def = params::all()[size_t(ModulationTarget { t }.paramIndex())];
            if (def.group != lastGroup) { target.addSectionHeading(def.group); lastGroup = def.group; }
            target.addItem(def.name, t + 2);
        }
        addAndMakeVisible(source);
        addAndMakeVisible(target);

        amount.setSliderStyle(juce::Slider::LinearHorizontal);
        amount.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        amount.setRange(-1.0, 1.0, 0.001);
        amount.setDoubleClickReturnValue(true, 0.0);
        amount.setPopupDisplayEnabled(true, true, nullptr);
        amount.setTooltip("Amount");
        addAndMakeVisible(amount);

        auto setupKnob = [this](juce::Slider& s, juce::Label& l, const char* name, double lo, double hi, double def, double skewMid)
        {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setRange(lo, hi, 0.001);
            if (skewMid > 0) s.setSkewFactorFromMidPoint(skewMid);
            s.setDoubleClickReturnValue(true, def);
            s.setPopupDisplayEnabled(true, true, nullptr);
            addAndMakeVisible(s);
            l.setText(name, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            styleSmall(l).setFont(juce::Font(juce::FontOptions(10.0f)));
            addAndMakeVisible(l);
            s.onValueChange = [this] { push(); };
        };
        setupKnob(minS, minL, "Min", 0, 1, 0, 0);
        setupKnob(maxS, maxL, "Max", 0, 1, 1, 0);
        setupKnob(smoothS, smoothL, "Smooth", 0, 1000, 30, 120);
        setupKnob(attackS, attackL, "Attack", 0, 2000, 5, 100);
        setupKnob(releaseS, releaseL, "Release", 0, 4000, 150, 300);
        setupKnob(curveS, curveL, "Curve", -1, 1, 0, 0);
        setupKnob(sensS, sensL, "Sens", 0, 4, 1, 1);
        minS.setTextValueSuffix(""); smoothS.setTextValueSuffix(" ms"); attackS.setTextValueSuffix(" ms"); releaseS.setTextValueSuffix(" ms");

        for (auto* b : { &bipolar, &invert })
        {
            b->setClickingTogglesState(true);
            addAndMakeVisible(b);
            b->onClick = [this] { push(); };
        }
        bipolar.setTooltip("Polarity: unipolar (0..1) / bipolar (-1..1)");
        invert.setTooltip("Invert the source");

        enabled.onClick = [this] { push(); };
        source.onChange = [this] { push(); };
        target.onChange = [this] { push(); };
        amount.onValueChange = [this] { push(); };
        refresh();
    }

    void refresh()
    {
        const juce::ScopedValueSetter<bool> svs(updating, true);
        const auto s = proc.matrix.getSlot(index);
        enabled.setToggleState(s.enabled, juce::dontSendNotification);
        source.setSelectedId(s.source + 1, juce::dontSendNotification);
        target.setSelectedId(s.target >= 0 ? s.target + 2 : 1, juce::dontSendNotification);
        amount.setValue(s.amount, juce::dontSendNotification);
        minS.setValue(s.min, juce::dontSendNotification);       maxS.setValue(s.max, juce::dontSendNotification);
        smoothS.setValue(s.smoothingMs, juce::dontSendNotification);
        attackS.setValue(s.attackMs, juce::dontSendNotification);
        releaseS.setValue(s.releaseMs, juce::dontSendNotification);
        curveS.setValue(s.curve, juce::dontSendNotification);   sensS.setValue(s.sensitivity, juce::dontSendNotification);
        bipolar.setToggleState(s.bipolar, juce::dontSendNotification);
        invert.setToggleState(s.invert, juce::dontSendNotification);
        const bool active = s.source > 0 && s.target >= 0;
        for (juce::Component* c : std::initializer_list<juce::Component*> { &amount, &minS, &maxS, &smoothS, &attackS, &releaseS, &curveS, &sensS, &bipolar, &invert })
            c->setAlpha(active ? 1.0f : 0.45f);
    }

    void push()
    {
        if (updating) return;
        ModSlot s;
        s.enabled = enabled.getToggleState();
        s.source = juce::jmax(0, source.getSelectedId() - 1);
        s.target = target.getSelectedId() >= 2 ? target.getSelectedId() - 2 : -1;
        s.amount = float(amount.getValue());
        s.min = float(minS.getValue()); s.max = float(maxS.getValue());
        s.smoothingMs = float(smoothS.getValue()); s.attackMs = float(attackS.getValue()); s.releaseMs = float(releaseS.getValue());
        s.curve = float(curveS.getValue()); s.sensitivity = float(sensS.getValue());
        s.bipolar = bipolar.getToggleState(); s.invert = invert.getToggleState();
        proc.matrix.setSlot(index, s);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(colours::panel2.withAlpha(0.6f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(6, 4);
        auto top = r.removeFromTop(24);
        number.setBounds(top.removeFromLeft(22));
        enabled.setBounds(top.removeFromLeft(36));
        const int w = (top.getWidth() - 6) / 2;
        source.setBounds(top.removeFromLeft(w - 20));
        top.removeFromLeft(6);
        target.setBounds(top.removeFromLeft(w + 20));
        auto amt = r.removeFromTop(18);
        amount.setBounds(amt.withTrimmedLeft(22));
        auto bottom = r;
        auto toggles = bottom.removeFromRight(40);
        bipolar.setBounds(toggles.removeFromTop(bottom.getHeight() / 2).reduced(1));
        invert.setBounds(toggles.reduced(1));
        juce::Slider* ks[] = { &minS, &maxS, &smoothS, &attackS, &releaseS, &curveS, &sensS };
        juce::Label* ls[] = { &minL, &maxL, &smoothL, &attackL, &releaseL, &curveL, &sensL };
        const int kw = bottom.getWidth() / 7;
        for (int i = 0; i < 7; ++i)
        {
            auto cell = juce::Rectangle<int>(bottom.getX() + i * kw, bottom.getY(), kw, bottom.getHeight());
            ls[i]->setBounds(cell.removeFromBottom(12));
            ks[i]->setBounds(cell);
        }
    }

private:
    DaliVisualProcessor& proc;
    const int index;
    bool updating = false;
    juce::Label number;
    juce::ToggleButton enabled;
    juce::ComboBox source, target;
    juce::Slider amount, minS, maxS, smoothS, attackS, releaseS, curveS, sensS;
    juce::Label minL, maxL, smoothL, attackL, releaseL, curveL, sensL;
    juce::TextButton bipolar { juce::String::fromUTF8("\xc2\xb1") }, invert { "INV" };
};

ModPanel::ModPanel(DaliVisualProcessor& p) : proc(p)
{
    addAndMakeVisible(header);
    addAndMakeVisible(clearAll);
    clearAll.onClick = [this] { proc.matrix.clear(); };
    for (int i = 0; i < kMaxModSlots; ++i) addAndMakeVisible(rows.add(new Row(p, i)));
    proc.matrix.addChangeListener(this);
}

ModPanel::~ModPanel() { proc.matrix.removeChangeListener(this); }

void ModPanel::changeListenerCallback(juce::ChangeBroadcaster*) { for (auto* r : rows) r->refresh(); }

int ModPanel::preferredHeight(int) { return kPad * 2 + 28 + kMaxModSlots * 96; }

void ModPanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    auto top = r.removeFromTop(24);
    clearAll.setBounds(top.removeFromRight(80));
    header.setBounds(top);
    r.removeFromTop(4);
    for (auto* row : rows) { row->setBounds(r.removeFromTop(92)); r.removeFromTop(4); }
}

// =============================================================================
//  FX
// =============================================================================
class FxPanel::Row : public juce::Component
{
public:
    Row(DaliVisualProcessor& p, int fxIndex)
        : proc(p), fx(fxIndex), info(effectLibrary()[size_t(fxIndex)]),
          on(p, params::id::fxOn(info.id), info.name),
          amount(p, params::id::fxAmt(info.id), info.p1Name),
          p2(p, params::id::fxP2(info.id), info.p2Name)
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &amount, &p2, &up, &down }) addAndMakeVisible(c);
        up.onClick   = [this] { move(-1); };
        down.onClick = [this] { move(+1); };
        up.setTooltip("Move earlier in the chain");
        down.setTooltip("Move later in the chain");
    }
    void move(int delta)
    {
        const auto order = proc.effectChain.getOrder();
        for (int i = 0; i < EffectChain::kNumEffects; ++i)
            if (order[size_t(i)] == fx) { proc.effectChain.move(i, delta); break; }
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(colours::panel2.withAlpha(0.6f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced(6, 3);
        auto arrows = r.removeFromRight(22);
        up.setBounds(arrows.removeFromTop(r.getHeight() / 2).reduced(1));
        down.setBounds(arrows.reduced(1));
        p2.setBounds(r.removeFromRight(62));
        amount.setBounds(r.removeFromRight(62));
        on.setBounds(r);
    }
private:
    DaliVisualProcessor& proc;
    const int fx;
    const EffectInfo& info;
    ParamToggle on;
    ParamKnob amount, p2;
    juce::TextButton up { juce::String::fromUTF8("\xe2\x96\xb2") }, down { juce::String::fromUTF8("\xe2\x96\xbc") };
};

FxPanel::FxPanel(DaliVisualProcessor& p) : proc(p)
{
    addAndMakeVisible(header);
    addAndMakeVisible(resetOrder);
    resetOrder.onClick = [this] { proc.effectChain.reset(); };
    for (int i = 0; i < EffectChain::kNumEffects; ++i) addAndMakeVisible(rows.add(new Row(p, i)));
    proc.effectChain.addChangeListener(this);
}

FxPanel::~FxPanel() { proc.effectChain.removeChangeListener(this); }

int FxPanel::preferredHeight(int) { return kPad * 2 + 28 + EffectChain::kNumEffects * 62; }

void FxPanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    auto top = r.removeFromTop(24);
    resetOrder.setBounds(top.removeFromRight(90));
    header.setBounds(top);
    r.removeFromTop(4);
    for (int e : proc.effectChain.getOrder()) { rows[e]->setBounds(r.removeFromTop(58)); r.removeFromTop(4); }
}

// =============================================================================
//  COLOR
// =============================================================================
class ColorPanel::Swatch : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    Swatch(DaliVisualProcessor& p, int idx) : proc(p), index(idx)
    {
        setTooltip(paletteNames()[idx]);
        startTimerHz(8);
    }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        const auto pal = index == int(PaletteId::Custom)
            ? makeCustomPalette(*proc.apvts.getRawParameterValue(params::id::customHueA), *proc.apvts.getRawParameterValue(params::id::customHueB))
            : builtInPalette(index);
        juce::Image img(juce::Image::RGB, 64, 1, false);
        for (int x = 0; x < 64; ++x)
        {
            float c[3];
            for (int i = 0; i < 3; ++i)
                c[i] = juce::jlimit(0.0f, 1.0f, pal.a[i] + pal.b[i] * std::cos(juce::MathConstants<float>::twoPi * (pal.c[i] * x / 63.0f + pal.d[i])));
            img.setPixelAt(x, 0, juce::Colour::fromFloatRGBA(c[0], c[1], c[2], 1.0f));
        }
        juce::Path clip; clip.addRoundedRectangle(r.withTrimmedBottom(14.0f), 5.0f);
        g.saveState();
        g.reduceClipRegion(clip);
        g.drawImage(img, r.withTrimmedBottom(14.0f), juce::RectanglePlacement::stretchToFit);
        g.restoreState();
        g.setColour(selected ? colours::accent : colours::outline);
        g.drawRoundedRectangle(r.withTrimmedBottom(14.0f), 5.0f, selected ? 2.0f : 1.0f);
        g.setColour(selected ? colours::text : colours::textDim);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawFittedText(paletteNames()[index], r.removeFromBottom(13.0f).toNearestInt(), juce::Justification::centred, 1);
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        if (auto* prm = proc.apvts.getParameter(params::id::palette))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(float(index)));
            prm->endChangeGesture();
        }
    }
private:
    void timerCallback() override
    {
        const bool s = juce::roundToInt(proc.apvts.getRawParameterValue(params::id::palette)->load()) == index;
        if (s != selected || index == int(PaletteId::Custom)) { selected = s; repaint(); }
    }
    DaliVisualProcessor& proc;
    const int index;
    bool selected = false;
};

ColorPanel::ColorPanel(DaliVisualProcessor& p)
    : proc(p),
      hue(p, params::id::hue, "Hue"), saturation(p, params::id::saturation, "Saturation"),
      brightness(p, params::id::brightness, "Brightness"), contrast(p, params::id::contrast, "Contrast"),
      colorAmount(p, params::id::colorAmount, "Color Amount"), colorShift(p, params::id::colorShift, "Color Shift"),
      audioColor(p, params::id::audioColor, "Audio Color"), customA(p, params::id::customHueA, "Base Hue"),
      customB(p, params::id::customHueB, "Highlight Hue")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &paletteHeader, &gradeHeader, &customHeader, &hue, &saturation, &brightness,
                     &contrast, &colorAmount, &colorShift, &audioColor, &customA, &customB })
        addAndMakeVisible(c);
    for (int i = 0; i < int(PaletteId::count); ++i) addAndMakeVisible(swatches.add(new Swatch(p, i)));
}

ColorPanel::~ColorPanel() = default;

int ColorPanel::preferredHeight(int) { return kPad * 4 + kHeaderH * 3 + 3 * 46 + kKnobH * 2 + kKnobH; }

void ColorPanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    paletteHeader.setBounds(r.removeFromTop(kHeaderH));
    auto grid = r.removeFromTop(3 * 46);
    const int sw = grid.getWidth() / 3;
    for (int i = 0; i < swatches.size(); ++i)
        swatches[i]->setBounds(grid.getX() + (i % 3) * sw, grid.getY() + (i / 3) * 46, sw, 46);
    r.removeFromTop(kPad);
    gradeHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &hue, &saturation, &brightness, &contrast, &colorAmount, &colorShift, &audioColor },
                   r.removeFromTop(kKnobH * 2), 4, kKnobH);
    r.removeFromTop(kPad);
    customHeader.setBounds(r.removeFromTop(kHeaderH));
    layoutKnobGrid({ &customA, &customB }, r.removeFromTop(kKnobH), 4, kKnobH);
}

// =============================================================================
//  IMAGE — Image Reactive Mode
// =============================================================================
ImagePanel::ImagePanel(DaliVisualProcessor& p)
    : proc(p),
      enable(p, params::id::tplEnable, "Template On"), mirror(p, "tplMirror", "Mirror"), kaleido(p, "tplKaleido", "Kaleidoscope"),
      mode(p, params::id::tplMode), blend(p, params::id::tplBlend)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &sourceHeader, &templateHeader, &structureHeader, &status, &loadBtn, &clearBtn,
                     &saveTpl, &loadTpl, &resetTpl, &routesBtn, &enable, &mirror, &kaleido, &mode, &blend })
        addAndMakeVisible(c);

    for (auto& id : params::templateParamIds())
        addAndMakeVisible(knobs.add(new ParamKnob(p, id, p.apvts.getParameter(id)->getName(20).replace("Template ", ""))));

    styleSmall(status);
    status.setJustificationType(juce::Justification::centred);
    status.setText(proc.image.getStatus(), juce::dontSendNotification);

    loadBtn.onClick  = [this] { chooseImage(); };
    clearBtn.onClick = [this] { proc.image.clear(); };
    saveTpl.onClick  = [this] { saveTemplate(); };
    loadTpl.onClick  = [this] { loadTemplate(); };
    resetTpl.onClick = [this] { proc.templates.resetParameters(); };
    routesBtn.onClick = [this] { TemplateGenerator::addReactiveRoutes(proc.matrix); };
    routesBtn.setTooltip("Adds Bass > Scale, Kick > Symmetry, Mid > Warp, High > Detail, Transient > Feedback, Sync LFO > Rotation");
    proc.image.addChangeListener(this);
}

ImagePanel::~ImagePanel() { proc.image.removeChangeListener(this); }

int ImagePanel::preferredHeight(int) { return kPad * 5 + kHeaderH * 3 + 150 + 20 + 30 * 4 + ((21 + 3) / 4) * kKnobH; }

void ImagePanel::resized()
{
    auto r = getLocalBounds().reduced(kPad);
    sourceHeader.setBounds(r.removeFromTop(kHeaderH));
    dropZone = r.removeFromTop(150);
    status.setBounds(r.removeFromTop(20));
    auto row = r.removeFromTop(30);
    loadBtn.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
    clearBtn.setBounds(row.reduced(2));
    r.removeFromTop(kPad);
    templateHeader.setBounds(r.removeFromTop(kHeaderH));
    row = r.removeFromTop(30);
    enable.setBounds(row.removeFromLeft(row.getWidth() / 3));
    mirror.setBounds(row.removeFromLeft(row.getWidth() / 2));
    kaleido.setBounds(row);
    row = r.removeFromTop(30);
    mode.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
    blend.setBounds(row.reduced(2));
    row = r.removeFromTop(30);
    const int bw = row.getWidth() / 4;
    saveTpl.setBounds(row.removeFromLeft(bw).reduced(2));
    loadTpl.setBounds(row.removeFromLeft(bw).reduced(2));
    resetTpl.setBounds(row.removeFromLeft(bw).reduced(2));
    routesBtn.setBounds(row.reduced(2));
    r.removeFromTop(kPad);
    structureHeader.setBounds(r.removeFromTop(kHeaderH));
    juce::Array<juce::Component*> ks;
    for (auto* k : knobs) ks.add(k);
    layoutKnobGrid(ks, r, 4, kKnobH);
}

void ImagePanel::paint(juce::Graphics& g)
{
    auto z = dropZone.toFloat().reduced(2.0f);
    g.setColour(colours::panel2);
    g.fillRoundedRectangle(z, 8.0f);
    juce::Path border; border.addRoundedRectangle(z, 8.0f);
    const float dash[] = { 6.0f, 4.0f };
    juce::Path dashed;
    juce::PathStrokeType(1.2f).createDashedStroke(dashed, border, dash, 2);
    g.setColour(proc.image.isBusy() ? colours::learn : colours::accent.withAlpha(0.7f));
    g.fillPath(dashed);

    const auto thumb = proc.image.getThumbnail();
    if (thumb.isValid())
        g.drawImage(thumb, z.reduced(10.0f), juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    else
    {
        g.setColour(colours::textDim);
        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        g.drawFittedText("Drop an image or logo here\n(or anywhere on the plug-in)", dropZone, juce::Justification::centred, 2);
    }
}

void ImagePanel::mouseUp(const juce::MouseEvent& e)
{
    if (dropZone.contains(e.getPosition())) chooseImage();
}

void ImagePanel::chooseImage()
{
    chooser = std::make_unique<juce::FileChooser>("Choose an image", juce::File(), "*.png;*.jpg;*.jpeg;*.gif;*.bmp");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile() && proc.image.loadFile(f))
                if (auto* prm = proc.apvts.getParameter(params::id::tplEnable)) prm->setValueNotifyingHost(1.0f);
        });
}

void ImagePanel::saveTemplate()
{
    chooser = std::make_unique<juce::FileChooser>("Save template", PresetManager::getPresetFolder().getSiblingFile("Templates"),
                                                  juce::String("*") + TemplateGenerator::fileExtension);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f != juce::File())
            {
                f.getParentDirectory().createDirectory();
                if (!proc.templates.saveToFile(f))
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Template", "Could not save the template.");
            }
        });
}

void ImagePanel::loadTemplate()
{
    chooser = std::make_unique<juce::FileChooser>("Load template", PresetManager::getPresetFolder().getSiblingFile("Templates"),
                                                  juce::String("*") + TemplateGenerator::fileExtension);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile() && !proc.templates.loadFromFile(f))
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Template", "Not a Dali Visual template.");
        });
}
} // namespace dali
