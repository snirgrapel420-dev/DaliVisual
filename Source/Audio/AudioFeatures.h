#pragma once
// ============================================================================
//  AudioFeatures — one analysis snapshot (framework independent).
//  All "normalised" values are 0..1 after adaptive gain control.
// ============================================================================
#include <cstdint>

namespace dali
{
struct AudioFeatures
{
    // level
    float rms = 0, peak = 0;                  // normalised (dB window)
    // bands (instant, normalised) and envelopes
    float low = 0, mid = 0, high = 0;
    float bassEnv = 0, midEnv = 0, highEnv = 0;
    float energy = 0;                          // overall energy envelope
    // events / envelopes
    float kick = 0;                            // kick envelope (1 on kick, decays)
    float transient = 0;                       // transient envelope
    float onset = 0;                           // onset pulse envelope
    // spectral
    float centroid = 0;                        // log-frequency position 0..1
    float flux = 0;                            // normalised spectral flux
    // stereo
    float width = 0;                           // 0 = mono, 1 = very wide
    float stereoEnergy = 0;                    // normalised energy of the side signal
    float pan = 0;                             // -1 left .. +1 right
    // tempo
    float bpm = 0;                             // detected BPM (0 = unknown)
    float bpmConfidence = 0;                   // 0..1
    float beatPhase = 0;                       // 0..1 at 'streamTime'
    std::uint32_t beatCount = 0;               // increments on each detected beat
    std::uint32_t kickCount = 0;
    std::uint32_t onsetCount = 0;
    // bookkeeping
    double streamTime = 0;                     // seconds of audio analysed
    bool   silent = true;
};
} // namespace dali
