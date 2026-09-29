#pragma once
// ============================================================================
//  AudioAnalyzer — bridges the audio thread and the analysis DSP.
//
//  Audio thread:    push()     → wait-free SPSC ring (never blocks / allocates)
//  Analysis thread: run()      → FeatureExtractor per 512-sample hop
//  Any thread:      snapshot() → latest AudioFeatures + time stamp
// ============================================================================
#include <juce_core/juce_core.h>
#include "FeatureExtractor.h"
#include "SpscRing.h"
#include <atomic>

namespace dali
{
class AudioAnalyzer : private juce::Thread
{
public:
    struct Snapshot
    {
        AudioFeatures features;
        double stamp = 0.0;          // seconds (now()) when the features were published
    };

    AudioAnalyzer();
    ~AudioAnalyzer() override;

    /** Setup thread. Safe to call while running. */
    void prepare(double sampleRate);
    /** Any thread (atomic). */
    void setSensitivity(float s) noexcept { sensitivity.store(s); }

    /** Audio thread. Wait-free. 'right' may be nullptr (mono). */
    void push(const float* left, const float* right, int numSamples) noexcept;

    /** Any non-audio thread. */
    Snapshot snapshot() const;

    static double now() noexcept { return juce::Time::getMillisecondCounterHiRes() * 0.001; }

private:
    struct Frame { float l, r; };
    void run() override;

    SpscRing<Frame> ring { 1u << 16 };
    FeatureExtractor extractor;
    std::atomic<double> pendingRate { 0.0 };
    std::atomic<float>  sensitivity { 1.0f };

    mutable juce::SpinLock lock;
    Snapshot latest;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalyzer)
};
} // namespace dali
