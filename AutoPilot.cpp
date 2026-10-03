#include "AutoPilot.h"
#include "../Render/Library.h"

namespace dali
{
void AutoPilot::timerCallback()
{
    const auto drops = state.analyzer.snapshot().features.dropCount;
    const int mode = juce::roundToInt(apvts.getRawParameterValue(params::id::autoPilot)->load());
    if (mode != 2 || state.telemetry.activity.load() < 0.5f)
    {
        lastPhrase = -1;
        lastDrops = drops;
        return;
    }

    static const int barsTable[] = { 2, 4, 8, 16, 32 };
    const int bars = barsTable[juce::jlimit(0, 4, juce::roundToInt(apvts.getRawParameterValue(params::id::autoBars)->load()))];
    const bool onDrop = apvts.getRawParameterValue(params::id::autoOnDrop)->load() > 0.5f;
    const std::int64_t bar = state.telemetry.barCount.load();
    const std::int64_t phrase = bar >= 0 ? bar / (std::int64_t(bars) * 4) : 0;

    if (lastPhrase < 0) { lastPhrase = phrase; lastDrops = drops; return; }

    const bool dropNow = onDrop && drops != lastDrops;
    if (phrase == lastPhrase && !dropNow) return;
    lastPhrase = phrase;
    lastDrops = drops;

    auto* p = apvts.getParameter(params::id::scene);
    if (p == nullptr) return;
    // the journey stays in its world family: the v4 worlds (18-23) among themselves, the earlier
    // scenes among themselves; the Image Reactor is never chosen automatically
    constexpr int firstWorld = 17;
    const int n = int(sceneLibrary().size());
    const int current = juce::roundToInt(p->convertFrom0to1(p->getValue()));
    juce::Array<int> pool;
    for (int i = 0; i < n; ++i)
        if (i != current && i != kImageSceneIndex && ((current >= firstWorld) == (i >= firstWorld))) pool.add(i);
    if (pool.isEmpty()) return;
    const int next = pool[random.nextInt(pool.size())];
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(float(next)));
    p->endChangeGesture();
}
} // namespace dali
