#pragma once
#include <array>
// ============================================================================
//  FeatureExtractor — the analysis DSP. Framework independent, allocation-free
//  after prepare(). Runs on the analysis thread (never on the audio thread).
//
//  Every hop (512 samples) it computes a 2048-point windowed FFT and derives:
//  RMS, peak, band energies + envelopes, kick, transient, onset, spectral
//  centroid, spectral flux, stereo width / energy / pan, BPM and beat phase.
//  Levels are normalised by an adaptive reference (AGC) so the visuals react
//  musically at any input level; 'sensitivity' scales the reaction.
// ============================================================================
#include "AudioFeatures.h"
#include "BeatTracker.h"
#include "FFT.h"
#include <complex>
#include <vector>

namespace dali
{
class FeatureExtractor
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;   // 2048
    static constexpr int hopSize  = 512;

    FeatureExtractor();
    void prepare(double sampleRate);
    void reset();

    void setSensitivity(float s) noexcept { sensitivity = s; }

    /** Feed exactly hopSize stereo samples. */
    void processHop(const float* left, const float* right);

    const AudioFeatures& features() const noexcept { return f; }
    double getSampleRate() const noexcept { return sr; }

private:
    struct Agc { double refDb = -30.0; float norm(double db, double rangeDb, double releaseDbPerHop); };
    struct OnsetDetector
    {
        std::vector<float> hist; size_t pos = 0; int refractory = 0;
        void prepare(int len) { hist.assign(size_t(len), 0.0f); pos = 0; refractory = 0; }
        bool process(float v, float k, int refractoryHops);
    };

    double sr = 48000.0, hopRate = 93.75;
    FFT fft { fftOrder };
    std::vector<float> ringL, ringR;   // last fftSize samples
    int ringPos = 0;
    std::vector<float> window;
    std::vector<std::complex<float>> buf;
    std::vector<float> mag, prevLogMag;

    int binLowA = 1, binLowB = 6, binKickA = 1, binKickB = 3, binMidB = 60, binHighB = 700;
    int binSnareA = 8, binSnareB = 170, binHatA = 256;

    Agc agcRms, agcLow, agcMid, agcHigh, agcSide;
    double fluxRef = 1e-3, hatRef = 1e-3;
    double sinceKick = 0.0, sinceDrop = 100.0;   // seconds of *non-silent* audio
    double fastDb = -100, slowDb = -100, prevKickDb = -100, kickAvg = -100, snAvg = -100, kickPeak = -70.0;

    OnsetDetector kickDet, onsetDet, snareDet, hatDet;
    BeatTracker beat;

    std::array<float, AudioFeatures::kSpectrumBands + 1> bandEdge {};   // fractional FFT bin of each band edge
    std::array<float, AudioFeatures::kSpectrumBands> bandTilt {};       // pink weighting (+3 dB/oct around 1 kHz)
    double specRefDb = -60.0;
    float wavePeak = 1e-3f;
    AudioFeatures f;
    float sensitivity = 1.0f;
    int   silentHops = 0;
};
} // namespace dali
