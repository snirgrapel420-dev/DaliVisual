#pragma once
// ============================================================================
//  AutoPilot — hands-free VJ mode.
//   Variations            macro / colour variations every N bars (done inside the
//                         render engine, deterministic per phrase)
//   Variations + Scenes   additionally switches to another scene every 4 phrases
//                         and (optionally) on every drop.
//  Only acts while music is playing. Runs on the message thread.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "EngineState.h"

namespace dali
{
class AutoPilot : private juce::Timer
{
public:
    AutoPilot(juce::AudioProcessorValueTreeState& s, EngineState& e) : apvts(s), state(e) { startTimerHz(10); }
    ~AutoPilot() override { stopTimer(); }

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    EngineState& state;
    std::int64_t lastPhrase = -1;
    std::uint32_t lastDrops = 0;
    juce::Random random;
};
} // namespace dali
