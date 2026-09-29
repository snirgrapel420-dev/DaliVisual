#include "PluginProcessor.h"
#include "PluginEditor.h"

DaliVisualProcessor::DaliVisualProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", dali::params::createLayout()),
      engineState(apvts, analyzer, matrix, effectChain, image),
      midi(apvts, engineState),
      output(engineState),
      templates(apvts, image),
      presets([this] { return captureState(false); }, [this](const juce::ValueTree& t) { applyState(t); })
{
    sensitivityParam = apvts.getRawParameterValue(dali::params::id::sensitivity);

    // Fresh instance: start from the Init preset's modulation (bass breathing on Macro A).
    matrix.addRoute(dali::ModSource::Bass, dali::ModulationTarget::fromParamId(dali::params::id::macroA), 0.25f);
}

DaliVisualProcessor::~DaliVisualProcessor()
{
    output.close();
}

bool DaliVisualProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out || in.isDisabled();
}

void DaliVisualProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    analyzer.prepare(sr);
}

void DaliVisualProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto t0 = juce::Time::getHighResolutionTicks();
    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch) buffer.clear(ch, 0, numSamples);

    // 1. analysis input (wait-free)
    if (numIn > 0 && numSamples > 0)
        analyzer.push(buffer.getReadPointer(0), numIn > 1 ? buffer.getReadPointer(1) : nullptr, numSamples);
    analyzer.setSensitivity(sensitivityParam->load());

    // 2. host transport
    dali::HostTiming ht;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            ht.valid = pos->getBpm().hasValue();
            ht.playing = pos->getIsPlaying();
            ht.bpm = pos->getBpm().orFallback(0.0);
            ht.ppq = pos->getPpqPosition().orFallback(0.0);
            ht.barStartPpq = pos->getPpqPositionOfLastBarStart().orFallback(0.0);
            if (auto sig = pos->getTimeSignature()) { ht.numerator = sig->numerator; ht.denominator = sig->denominator; }
            ht.stamp = dali::AudioAnalyzer::now();
        }
    }
    engineState.host.writeFromAudioThread(ht);

    // 3. MIDI
    for (const auto metadata : midiMessages)
        midi.pushFromAudioThread(metadata.getMessage());

    // Standalone: visual instrument only — never send the input back to the speakers.
    if (isStandalone()) buffer.clear();

    // audio-thread load (for the CPU meter)
    const double elapsed = juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - t0);
    const double budget = numSamples / juce::jmax(1.0, sampleRate);
    loadSmoothed += (float(elapsed / juce::jmax(1e-6, budget)) - loadSmoothed) * 0.05f;
    engineState.telemetry.cpuLoad.store(loadSmoothed);
}

// =============================================================================
//  state
// =============================================================================
juce::ValueTree DaliVisualProcessor::captureState(bool includeGlobal) const
{
    juce::ValueTree st(stateId);
    st.setProperty("version", 1, nullptr);

    juce::ValueTree p("Params");
    for (auto& d : dali::params::all())
        if (auto* prm = apvts.getParameter(d.id))
            p.setProperty(d.id, prm->convertFrom0to1(prm->getValue()), nullptr);
    st.appendChild(p, nullptr);

    st.appendChild(matrix.toValueTree(), nullptr);
    st.appendChild(effectChain.toValueTree(), nullptr);

    juce::ValueTree o("Output");
    o.setProperty("renderScale", engineState.output.renderScaleIndex.load(), nullptr);
    o.setProperty("vsync", engineState.output.vsync.load(), nullptr);
    o.setProperty("previewWhileOutput", engineState.output.previewWhileOutput.load(), nullptr);
    o.setProperty("display", engineState.output.displayIndex.load(), nullptr);
    st.appendChild(o, nullptr);

    if (image.hasImage())
    {
        const auto png = image.getEncodedPNG();
        juce::ValueTree img("Image");
        img.setProperty("name", image.getName(), nullptr);
        img.setProperty("png", juce::Base64::toBase64(png.getData(), png.getSize()), nullptr);
        st.appendChild(img, nullptr);
    }

    if (includeGlobal)
    {
        st.appendChild(midi.toValueTree(), nullptr);
        st.setProperty("preset", presets.getCurrentName(), nullptr);
    }
    return st;
}

void DaliVisualProcessor::applyState(const juce::ValueTree& st)
{
    if (!st.hasType(stateId)) return;

    const auto p = st.getChildWithName("Params");
    for (auto& d : dali::params::all())
        if (auto* prm = apvts.getParameter(d.id))
        {
            const float norm = p.hasProperty(d.id) ? prm->convertTo0to1(float(p.getProperty(d.id)))
                                                    : prm->getDefaultValue();
            prm->setValueNotifyingHost(norm);
        }

    matrix.fromValueTree(st.getChildWithName(dali::ModulationMatrix::treeId));
    effectChain.fromValueTree(st.getChildWithName(dali::EffectChain::treeId));

    const auto o = st.getChildWithName("Output");
    if (o.isValid())
    {
        engineState.output.renderScaleIndex = int(o.getProperty("renderScale", 2));
        engineState.output.vsync = bool(o.getProperty("vsync", true));
        engineState.output.previewWhileOutput = bool(o.getProperty("previewWhileOutput", true));
        engineState.output.displayIndex = int(o.getProperty("display", -1));
    }

    const auto img = st.getChildWithName("Image");
    if (img.isValid())
    {
        juce::MemoryOutputStream out;
        if (juce::Base64::convertFromBase64(out, img.getProperty("png").toString()))
            image.loadEncoded(out.getMemoryBlock(), img.getProperty("name", "Image").toString());
    }
    // No image in the preset: keep the current source image (source and template are separate).

    const auto m = st.getChildWithName(dali::MidiMapper::treeId);
    if (m.isValid()) midi.fromValueTree(m);
    if (st.hasProperty("preset")) presets.setCurrentName(st.getProperty("preset").toString());
}

void DaliVisualProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = captureState(true).createXml())
        copyXmlToBinary(*xml, destData);
}

void DaliVisualProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) applyState(tree);
        else juce::MessageManager::callAsync([this, tree] { applyState(tree); });
    }
}

juce::AudioProcessorEditor* DaliVisualProcessor::createEditor()
{
    return new DaliVisualEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DaliVisualProcessor();
}
