#include "AudioAnalyzer.h"

namespace dali
{
AudioAnalyzer::AudioAnalyzer() : juce::Thread("DaliVisual Analysis")
{
    startThread(juce::Thread::Priority::high);
}

AudioAnalyzer::~AudioAnalyzer()
{
    stopThread(2000);
}

void AudioAnalyzer::prepare(double sampleRate)
{
    pendingRate.store(sampleRate);            // applied by the analysis thread
}

void AudioAnalyzer::push(const float* left, const float* right, int numSamples) noexcept
{
    Frame tmp[256];
    int pos = 0;
    while (pos < numSamples)
    {
        const int n = juce::jmin(256, numSamples - pos);
        for (int i = 0; i < n; ++i) tmp[i] = { left[pos + i], right != nullptr ? right[pos + i] : left[pos + i] };
        ring.push(tmp, size_t(n));            // if the analyzer falls behind, excess is dropped (never blocks)
        pos += n;
    }
}

AudioAnalyzer::Snapshot AudioAnalyzer::snapshot() const
{
    const juce::SpinLock::ScopedLockType sl(lock);
    return latest;
}

void AudioAnalyzer::run()
{
    constexpr int H = FeatureExtractor::hopSize;
    Frame frames[H];
    float L[H], R[H];

    while (!threadShouldExit())
    {
        const double rate = pendingRate.exchange(0.0);
        if (rate > 0.0) extractor.prepare(rate);

        bool worked = false;
        while (ring.available() >= size_t(H) && !threadShouldExit())
        {
            ring.pop(frames, size_t(H));
            for (int i = 0; i < H; ++i) { L[i] = frames[i].l; R[i] = frames[i].r; }
            extractor.setSensitivity(sensitivity.load());
            extractor.processHop(L, R);

            const juce::SpinLock::ScopedLockType sl(lock);
            latest.features = extractor.features();
            latest.stamp = now();
            worked = true;
        }
        if (!worked) wait(2);
    }
}
} // namespace dali
