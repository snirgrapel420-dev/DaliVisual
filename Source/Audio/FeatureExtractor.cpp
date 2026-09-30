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
    binSnareA = bin(180); binSnareB = bin(4000); binHatA = std::min(bin(7000), binHighB - 1);

    // 128 log-spaced analysis bands, 30 Hz .. 16 kHz (capped below Nyquist)
    {
        const double fLo = 30.0, fHi = std::min(16000.0, sr * 0.45);
        for (int i = 0; i <= AudioFeatures::kSpectrumBands; ++i)
            bandEdge[size_t(i)] = float(fLo * std::pow(fHi / fLo, double(i) / AudioFeatures::kSpectrumBands) / binHz);
        for (int i = 0; i < AudioFeatures::kSpectrumBands; ++i)
        {
            const double fc = std::sqrt(double(bandEdge[size_t(i)]) * bandEdge[size_t(i + 1)]) * binHz;
            bandTilt[size_t(i)] = float(3.0 * std::log2(fc / 1000.0));
        }
    }
    kickDet.prepare(int(hopRate * 1.0));
    onsetDet.prepare(int(hopRate * 0.8));
    snareDet.prepare(int(hopRate * 0.8));
    hatDet.prepare(int(hopRate * 0.5));
    beat.prepare(hopRate);
    reset();
}

void FeatureExtractor::reset()
{
    std::fill(ringL.begin(), ringL.end(), 0.0f); std::fill(ringR.begin(), ringR.end(), 0.0f);
    std::fill(prevLogMag.begin(), prevLogMag.end(), 0.0f);
    agcRms = agcLow = agcMid = agcHigh = agcSide = Agc {};
    fluxRef = hatRef = 1e-3; fastDb = slowDb = prevKickDb = kickAvg = -100;
    sinceKick = 0.0; sinceDrop = 100.0; snAvg = -100; kickPeak = -70.0;
    specRefDb = -60.0; wavePeak = 1e-3f;
    kickDet.prepare(int(hopRate * 1.0)); onsetDet.prepare(int(hopRate * 0.8));
    snareDet.prepare(int(hopRate * 0.8)); hatDet.prepare(int(hopRate * 0.5));
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
    double flux = 0, hatFlux = 0;
    for (int k = binLowA; k < binHighB; ++k)
    {
        const float lm = std::log1p(1000.0f * mag[size_t(k)]);
        const float d = std::max(0.0f, lm - prevLogMag[size_t(k)]);
        flux += d;
        if (k >= binHatA) hatFlux += d;
        prevLogMag[size_t(k)] = lm;
    }
    flux /= std::max(1, binHighB - binLowA);
    hatFlux /= std::max(1, binHighB - binHatA);
    hatRef = std::max(hatFlux, hatRef * 0.9995);
    const float hatN = f.silent ? 0.0f : clamp01(float(hatFlux / std::max(hatRef, 1e-4)));
    fluxRef = std::max(flux, fluxRef * 0.9995);
    const float fluxN = f.silent ? 0.0f : clamp01(float(flux / std::max(fluxRef, 1e-4)));
    f.flux = follow(f.flux, shape(fluxN), coef(0.004, hopRate), coef(0.12, hopRate));

    // ---- kick: rise in the kick band against its short-term average -----------
    const float kickOdf = float(std::max(0.0, kickDb - kickAvg)) / 12.0f;
    kickAvg = kickAvg < -99 ? kickDb : kickAvg + (kickDb - kickAvg) * 0.25;
    // kick-band peak memory (instant attack, 0.5 dB/s release): a kick must reach
    // within 12 dB of recent kicks, so noise/risers in a breakdown are not kicks
    kickPeak = kickDb > kickPeak ? kickDb : std::max(-70.0, kickPeak - 0.5 / hopRate);
    const bool kickStrong = kickDb > kickPeak - 12.0;
    // gate the *input* (not the result): a rejected candidate must not start the refractory period
    const bool kickHit = !f.silent && kickDet.process((f.low > 0.3f && kickStrong) ? kickOdf : 0.0f, 1.6f, int(hopRate * 0.09));
    if (kickHit) { f.kick = 1.0f; ++f.kickCount; }
    else f.kick *= 1.0f - coef(0.13, hopRate);

    // ---- snare / clap: mid-band noise burst that is not just the kick's click ----
    //      (level rise in 180 Hz-4 kHz, weighted by spectral flatness: noise is flat,
    //       a side-chained bass or synth swelling back in is harmonic → rejected)
    const double snDb = toDb(bandRms(binSnareA, binSnareB));
    const float snRise = float(std::max(0.0, snDb - snAvg)) / 12.0f;
    snAvg = snAvg < -99 ? snDb : snAvg + (snDb - snAvg) * 0.25;
    double logSum = 0, linSum = 0;
    for (int k = binSnareA; k < binSnareB; ++k) { logSum += std::log(double(mag[size_t(k)]) + 1e-12); linSum += mag[size_t(k)]; }
    const int nSn = std::max(1, binSnareB - binSnareA);
    const float flat = linSum > 1e-12 ? float(std::exp(logSum / nSn) / (linSum / nSn)) : 0.0f;
    const float noisy = clamp01((flat - 0.62f) / 0.15f);
    const float snOdf = snRise * noisy * (1.0f - 0.8f * std::min(1.0f, kickOdf * 1.5f));
    // adaptive threshold (relative to recent history) AND an absolute one: periodic
    // non-snare events must not become 'snares' just because they repeat
    const bool snareHit = !f.silent && snareDet.process((f.mid > 0.25f && !kickHit) ? snOdf : 0.0f, 1.8f, int(hopRate * 0.12))
                          && snOdf > 0.3f;
    if (snareHit) { f.snare = 1.0f; ++f.snareCount; }
    else f.snare *= 1.0f - coef(0.15, hopRate);

    // ---- hi-hat: high-band flux hits ----------------------------------------------
    const bool hatHit = !f.silent && hatDet.process(f.high > 0.25f ? hatN : 0.0f, 1.4f, int(hopRate * 0.05));
    if (hatHit) { f.hat = 1.0f; ++f.hatCount; }
    else f.hat *= 1.0f - coef(0.06, hopRate);

    // ---- musical structure: build (no kick for a while) and drop (kick returns) --
    const double hopSec = hopSize / sr;
    if (!f.silent)
    {
        sinceDrop += hopSec;
        if (kickHit)
        {
            if (sinceKick >= 4.0 && sinceDrop > 8.0) { f.drop = 1.0f; ++f.dropCount; sinceDrop = 0.0; }
            sinceKick = 0.0;
        }
        else sinceKick += hopSec;
    }
    // ramps up from 1.5 s to 9 s without a kick, fades out again for very long kick-less passages (ambient)
    const double ramp = std::clamp((sinceKick - 1.5) / 7.5, 0.0, 1.0) * std::clamp((40.0 - sinceKick) / 16.0, 0.0, 1.0);
    const float buildTarget = f.silent ? 0.0f : float(ramp) * (0.45f + 0.55f * f.energy);
    f.build = follow(f.build, buildTarget, coef(0.6, hopRate), coef(sinceKick < 0.5 ? 0.08 : 0.8, hopRate));
    f.drop *= 1.0f - coef(1.5, hopRate);

    // ---- activity gate: 1 while music plays, 0 in silence (drives motion) ----------
    f.activity = follow(f.activity, f.silent ? 0.0f : 1.0f, coef(0.12, hopRate), coef(0.5, hopRate));

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

    // ---- full spectrum (for the GPU) ------------------------------------------------
    {
        double hopMax = -120.0;
        std::array<double, AudioFeatures::kSpectrumBands> db {};
        const int nb = fftSize / 2;
        for (int i = 0; i < AudioFeatures::kSpectrumBands; ++i)
        {
            const float lo = bandEdge[size_t(i)], hi = bandEdge[size_t(i + 1)];
            double e;
            if (hi - lo < 1.0f)                           // narrower than a bin: interpolate the magnitude
            {
                const float c = 0.5f * (lo + hi);
                const int k = std::clamp(int(c), 0, nb - 1);
                const float t = c - float(k);
                const double m = mag[size_t(k)] * (1.0f - t) + mag[size_t(std::min(k + 1, nb))] * t;
                e = m * m;
            }
            else
            {
                const int a = std::clamp(int(std::floor(lo)), 0, nb), b = std::clamp(int(std::ceil(hi)), a + 1, nb + 1);
                double sum = 0; for (int k = a; k < b; ++k) sum += double(mag[size_t(k)]) * mag[size_t(k)];
                e = sum / (b - a);
            }
            db[size_t(i)] = toDb(std::sqrt(e)) + bandTilt[size_t(i)];
            hopMax = std::max(hopMax, db[size_t(i)]);
        }
        specRefDb = hopMax > specRefDb ? hopMax : std::max(hopMax, specRefDb - 0.03);   // auto-level, slow release
        specRefDb = std::max(specRefDb, -70.0);
        const float rel = 1.0f - coef(0.12, hopRate);
        for (int i = 0; i < AudioFeatures::kSpectrumBands; ++i)
        {
            const float v = f.silent ? 0.0f : clamp01(float((db[size_t(i)] - (specRefDb - 42.0)) / 42.0) * sens);
            float& o = f.spectrum[size_t(i)];
            o = v > o ? v : o * rel;
        }
    }

    // ---- waveform (oscilloscope), auto-levelled ------------------------------------------
    {
        float pkHop = 1e-6f;
        for (int i = 0; i < hopSize; ++i) pkHop = std::max(pkHop, std::abs(0.5f * (left[i] + right[i])));
        wavePeak = std::max(pkHop, wavePeak * 0.995f);
        const int step = hopSize / AudioFeatures::kWaveSamples;
        for (int i = 0; i < AudioFeatures::kWaveSamples; ++i)
        {
            // peak-preserving decimation: keep the sample with the largest |x| in each bucket
            // (plain decimation aliases high frequencies away)
            float m = 0.0f;
            for (int j = 0; j < step; ++j)
            {
                const float x = 0.5f * (left[i * step + j] + right[i * step + j]);
                if (std::abs(x) > std::abs(m)) m = x;
            }
            f.wave[size_t(i)] = f.silent ? 0.0f : std::clamp(m / wavePeak, -1.0f, 1.0f);
        }
    }

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
