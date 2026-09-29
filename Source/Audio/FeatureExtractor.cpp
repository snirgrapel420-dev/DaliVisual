#include "FeatureExtractor.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace dali
{
namespace
{
inline double toDb(double v) { return 20.0 * std::log10(std::max(v, 1e-9)); }
inline float  coef(double timeSec, double hopRate) { return float(1.0 - std::exp(-1.0 / std::max(1e-4, timeSec * hopRate))); }
inline float  follow(float cur, float target, float att, float rel) { return cur + (target - cur) * (target > cur ? att : rel); }
inline float  clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
}

float FeatureExtractor::Agc::norm(double db, double rangeDb, double releaseDbPerHop)
{
    if (db > refDb) refDb = db;                                    // instant attack
    else refDb = std::max(db, refDb - releaseDbPerHop);           // slow release
    refDb = std::max(refDb, -60.0);                                // noise floor: never boost silence
    return clamp01(float((db - (refDb - rangeDb)) / rangeDb));
}

bool FeatureExtractor::OnsetDetector::process(float v, float k, int refractoryHops)
{
    if (hist.empty()) return false;
    const float mean = std::accumulate(hist.begin(), hist.end(), 0.0f) / float(hist.size());
    float var = 0; for (float h : hist) var += (h - mean) * (h - mean);
    const float sd = std::sqrt(var / float(hist.size()));
    hist[pos] = v; pos = (pos + 1) % hist.size();
    if (refractory > 0) { --refractory; return false; }
    if (v > mean + k * sd && v > 0.02f) { refractory = refractoryHops; return true; }
    return false;
}

FeatureExtractor::FeatureExtractor() { prepare(48000.0); }

void FeatureExtractor::prepare(double sampleRate)
{
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    hopRate = sr / hopSize;
    ringL.assign(fftSize, 0.0f); ringR.assign(fftSize, 0.0f); ringPos = 0;
    window.resize(fftSize);
    for (int i = 0; i < fftSize; ++i) window[size_t(i)] = float(0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * i / (fftSize - 1)));
    buf.assign(fftSize, {});
    mag.assign(fftSize / 2 + 1, 0.0f);
    prevLogMag.assign(fftSize / 2 + 1, 0.0f);

    const double binHz = sr / fftSize;
    auto bin = [&](double hz) { return std::clamp(int(std::lround(hz / binHz)), 1, fftSize / 2); };
    binLowA = bin(25); binLowB = bin(160); binKickA = bin(38); binKickB = bin(125);
    binMidB = bin(2500); binHighB = bin(std::min(16000.0, sr * 0.45));

    kickDet.prepare(int(hopRate * 1.0));
    onsetDet.prepare(int(hopRate * 0.8));
    beat.prepare(hopRate);
    reset();
}

void FeatureExtractor::reset()
{
    std::fill(ringL.begin(), ringL.end(), 0.0f); std::fill(ringR.begin(), ringR.end(), 0.0f);
    std::fill(prevLogMag.begin(), prevLogMag.end(), 0.0f);
    agcRms = agcLow = agcMid = agcHigh = agcSide = Agc {};
    fluxRef = 1e-3; fastDb = slowDb = prevKickDb = kickAvg = -100;
    kickDet.prepare(int(hopRate * 1.0)); onsetDet.prepare(int(hopRate * 0.8));
    beat.reset();
    f = AudioFeatures {};
    silentHops = 0;
}

void FeatureExtractor::processHop(const float* left, const float* right)
{
    // ---- time domain: level + stereo ----------------------------------------
    double sumM = 0, sumS = 0, sumL = 0, sumR = 0; float pk = 0;
    for (int i = 0; i < hopSize; ++i)
    {
        const float l = left[i], r = right[i];
        const float m = 0.5f * (l + r), s = 0.5f * (l - r);
        sumM += double(m) * m; sumS += double(s) * s; sumL += double(l) * l; sumR += double(r) * r;
        pk = std::max(pk, std::max(std::abs(l), std::abs(r)));
        ringL[size_t(ringPos)] = l; ringR[size_t(ringPos)] = r;
        ringPos = (ringPos + 1) % fftSize;
    }
    const double rmsM = std::sqrt(sumM / hopSize), rmsS = std::sqrt(sumS / hopSize);
    const double rmsL = std::sqrt(sumL / hopSize), rmsR = std::sqrt(sumR / hopSize);
    const double rmsDb = toDb(rmsM);

    const bool silentNow = rmsDb < -62.0;
    silentHops = silentNow ? silentHops + 1 : 0;
    f.silent = silentHops > int(hopRate * 0.25);

    const float sens = std::max(0.0f, sensitivity);
    auto shape = [&](float v) { return clamp01(v * sens); };

    f.rms  = shape(agcRms.norm(rmsDb, 36.0, 0.04));
    f.peak = shape(clamp01(float((toDb(pk) + 48.0) / 48.0)));

    // ---- spectrum ------------------------------------------------------------
    for (int i = 0; i < fftSize; ++i)
    {
        const size_t idx = size_t((ringPos + i) % fftSize);
        buf[size_t(i)] = { 0.5f * (ringL[idx] + ringR[idx]) * window[size_t(i)], 0.0f };
    }
    fft.forward(buf.data());
    const float scale = 2.0f / (fftSize * 0.5f);
    for (int k = 0; k <= fftSize / 2; ++k) mag[size_t(k)] = std::abs(buf[size_t(k)]) * scale;

    auto bandRms = [&](int a, int b) { double s = 0; for (int k = a; k < b; ++k) s += double(mag[size_t(k)]) * mag[size_t(k)]; return std::sqrt(s / std::max(1, b - a)); };
    const double lowDb  = toDb(bandRms(binLowA, binLowB));
    const double midDb  = toDb(bandRms(binLowB, binMidB));
    const double highDb = toDb(bandRms(binMidB, binHighB)) + 12.0;     // tilt: highs carry less energy
    const double kickDb = toDb(bandRms(binKickA, binKickB));

    f.low  = f.silent ? 0.0f : shape(agcLow.norm(lowDb, 30.0, 0.05));
    f.mid  = f.silent ? 0.0f : shape(agcMid.norm(midDb, 30.0, 0.05));
    f.high = f.silent ? 0.0f : shape(agcHigh.norm(highDb, 30.0, 0.05));

    f.bassEnv = follow(f.bassEnv, f.low,  coef(0.005, hopRate), coef(0.18, hopRate));
    f.midEnv  = follow(f.midEnv,  f.mid,  coef(0.010, hopRate), coef(0.14, hopRate));
    f.highEnv = follow(f.highEnv, f.high, coef(0.003, hopRate), coef(0.09, hopRate));
    f.energy  = follow(f.energy, 0.45f * f.bassEnv + 0.35f * f.midEnv + 0.2f * f.highEnv, coef(0.02, hopRate), coef(0.35, hopRate));

    // centroid (log frequency 80 Hz .. 12 kHz → 0..1)
    double num = 0, den = 0;
    const double binHz = sr / fftSize;
    for (int k = binLowA; k < binHighB; ++k) { num += mag[size_t(k)] * (k * binHz); den += mag[size_t(k)]; }
    const float cNow = den > 1e-9 ? clamp01(float(std::log2(std::max(80.0, num / den) / 80.0) / std::log2(12000.0 / 80.0))) : f.centroid;
    f.centroid = follow(f.centroid, cNow, coef(0.05, hopRate), coef(0.25, hopRate));

    // spectral flux (half-wave rectified log-magnitude difference)
    double flux = 0;
    for (int k = binLowA; k < binHighB; ++k)
    {
        const float lm = std::log1p(1000.0f * mag[size_t(k)]);
        flux += std::max(0.0f, lm - prevLogMag[size_t(k)]);
        prevLogMag[size_t(k)] = lm;
    }
    flux /= std::max(1, binHighB - binLowA);
    fluxRef = std::max(flux, fluxRef * 0.9995);
    const float fluxN = f.silent ? 0.0f : clamp01(float(flux / std::max(fluxRef, 1e-4)));
    f.flux = follow(f.flux, shape(fluxN), coef(0.004, hopRate), coef(0.12, hopRate));

    // ---- kick: rise in the kick band against its short-term average -----------
    const float kickOdf = float(std::max(0.0, kickDb - kickAvg)) / 12.0f;
    kickAvg = kickAvg < -99 ? kickDb : kickAvg + (kickDb - kickAvg) * 0.25;
    const bool kickHit = !f.silent && kickDet.process(kickOdf, 1.6f, int(hopRate * 0.09)) && f.low > 0.35f;
    if (kickHit) { f.kick = 1.0f; ++f.kickCount; }
    else f.kick *= 1.0f - coef(0.13, hopRate);

    // ---- onset (broadband flux) -------------------------------------------------
    const bool onsetHit = !f.silent && onsetDet.process(fluxN, 1.4f, int(hopRate * 0.06));
    if (onsetHit) { f.onset = 1.0f; ++f.onsetCount; }
    else f.onset *= 1.0f - coef(0.10, hopRate);

    // ---- transient: fast vs slow level ------------------------------------------
    fastDb = fastDb < -99 ? rmsDb : fastDb + (rmsDb - fastDb) * 0.7;
    slowDb = slowDb < -99 ? rmsDb : slowDb + (rmsDb - slowDb) * coef(0.25, hopRate);
    const float tr = f.silent ? 0.0f : shape(clamp01(float((fastDb - slowDb) / 9.0)));
    f.transient = std::max(tr, f.transient * (1.0f - coef(0.08, hopRate)));

    // ---- stereo -------------------------------------------------------------------
    f.width = clamp01(float(rmsS / std::max(1e-9, rmsM + rmsS)) * 2.0f);
    f.stereoEnergy = f.silent ? 0.0f : shape(agcSide.norm(toDb(rmsS), 36.0, 0.04));
    const float panNow = float((rmsR - rmsL) / std::max(1e-9, rmsR + rmsL));
    f.pan = follow(f.pan, panNow, coef(0.05, hopRate), coef(0.05, hopRate));

    // ---- tempo / beat -------------------------------------------------------------
    const float odf = fluxN * 0.6f + std::min(kickOdf, 2.0f) * 0.8f;
    beat.process(odf, !f.silent);
    f.bpm = beat.bpm();
    f.bpmConfidence = beat.confidence();
    f.beatPhase = beat.phase();
    f.beatCount = beat.beats();

    f.streamTime += hopSize / sr;
}
} // namespace dali
