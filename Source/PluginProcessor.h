#pragma once
// ============================================================================
//  DALI VISUAL — audio processor (shared by VST3 and Standalone).
//
//  The audio thread does only three cheap, wait-free things:
//    1. pushes the input into the analyzer FIFO,
//    2. publishes host transport (try-lock, never blocks),
//    3. forwards MIDI into the MIDI-mapper FIFO.
//  Audio passes through untouched in the plug-in; the Standalone silences its
//  output (it is a visual instrument listening to an input — no feedback).
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "Core/EngineState.h"
#include "Core/Parameters.h"
#include "Audio/AudioAnalyzer.h"
#include "Modulation/ModulationMatrix.h"
#include "Render/EffectChain.h"
#include "Image/ImageProcessor.h"
#include "Image/TemplateGenerator.h"
#include "Midi/MidiMapper.h"
#include "Output/OutputManager.h"
#include "Preset/PresetManager.h"

class DaliVisualProcessor : public juce::AudioProcessor
{
public:
    DaliVisualProcessor();
    ~DaliVisualProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    /** Full instrument state. includeGlobal adds MIDI mappings + preset name (plug-in state only). */
    juce::ValueTree captureState(bool includeGlobal) const;
    void applyState(const juce::ValueTree& state);

    bool isStandalone() const noexcept { return wrapperType == wrapperType_Standalone; }

    // ---- accessors for the UI ---------------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    dali::AudioAnalyzer     analyzer;
    dali::ModulationMatrix  matrix;
    dali::EffectChain       effectChain;
    dali::ImageProcessor    image;
    dali::EngineState       engineState;
    dali::MidiMapper        midi;
    dali::OutputManager     output;
    dali::TemplateGenerator templates;
    dali::PresetManager     presets;

    static inline const juce::Identifier stateId { "DaliVisualState" };

private:
    std::atomic<float>* sensitivityParam = nullptr;
    double sampleRate = 48000.0;
    float loadSmoothed = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DaliVisualProcessor)
};
