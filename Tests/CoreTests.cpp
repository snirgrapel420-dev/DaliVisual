// ============================================================================
//  DALI VISUAL — core unit tests (no JUCE required)
//  Build: see Tests/CMakeLists.txt, or:
//    g++ -O2 -std=c++17 -pthread CoreTests.cpp ../Source/Audio/*.cpp  +
//        ../Source/Modulation/ModulationCore.cpp ../Source/Image/ImageDNA.cpp -o core_tests (see Tests/CMakeLists.txt)
// ============================================================================
#include "../Source/Audio/FeatureExtractor.h"
#include "../Source/Audio/MusicalClock.h"
#include "../Source/Audio/SpscRing.h"
#include "../Source/Modulation/ModulationCore.h"
#include "../Source/Image/ImageDNA.h"
#include <chrono>
#include <functional>
#include <cmath>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++failures; std::printf("  FAIL %s:%d  ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static const double PI = 3.14159265358979323846;

// ---------------------------------------------------------------------------
struct Track { std::vector<float> L, R; std::vector<double> kickTimes; };

static Track makeTrack(double bpm, double seconds, double sr, float gain = 1.0f, unsigned seed = 1)
{
    Track t; const size_t n = size_t(seconds * sr);
    t.L.assign(n, 0.0f); t.R.assign(n, 0.0f);
    std::mt19937 rng(seed); std::uniform_real_distribution<float> U(-1.0f, 1.0f);
    const double beat = 60.0 / bpm;
    double kickPhase = 0, bassPhase = 0; float prevNoise = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const double time = i / sr;
        const double bpos = std::fmod(time, beat);
        const double hpos = std::fmod(time + beat * 0.5, beat);
        if (i > 0 && bpos < 1.0 / sr) t.kickTimes.push_back(time);
        // kick: pitch sweep 150 → 48 Hz
        const double kf = 48.0 + 102.0 * std::exp(-bpos * 30.0);
        kickPhase += 2 * PI * kf / sr;
        const float kick = float(std::sin(kickPhase) * std::exp(-bpos * 9.0)) * 0.85f;
        // off-beat hat (high-passed noise)
        const float nz = U(rng); const float hp = nz - prevNoise; prevNoise = nz;
        const float hat = hp * float(std::exp(-hpos * 60.0)) * 0.18f;
        // rolling bass, side-chained
        bassPhase += 2 * PI * 55.0 / sr;
        const float duck = float(1.0 - std::exp(-bpos * 12.0));
        const float bass = float(std::fmod(bassPhase / (2 * PI), 1.0) * 2.0 - 1.0) * 0.12f * duck;
        const float pad = U(rng) * 0.01f;
        const float m = (kick + hat + bass + pad) * gain;
        t.L[i] = m + hat * 0.3f * gain; t.R[i] = m - hat * 0.3f * gain;
    }
    return t;
}

static void runExtractor(dali::FeatureExtractor& fx, const Track& t,
                         const std::function<void(const dali::AudioFeatures&)>& perHop = {})
{
    const int H = dali::FeatureExtractor::hopSize;
    for (size_t i = 0; i + H <= t.L.size(); i += H)
    {
        fx.processHop(&t.L[i], &t.R[i]);
        if (perHop) perHop(fx.features());
    }
}

// ---------------------------------------------------------------------------
static void testFFT()
{
    std::printf("[FFT]\n");
    dali::FFT fft(10);
    std::vector<std::complex<float>> d(1024);
    for (int i = 0; i < 1024; ++i) d[size_t(i)] = { float(std::sin(2 * PI * 37 * i / 1024.0)), 0.0f };
    fft.forward(d.data());
    int best = 0; for (int k = 1; k < 512; ++k) if (std::abs(d[size_t(k)]) > std::abs(d[size_t(best)])) best = k;
    CHECK(best == 37, "peak bin %d", best);
    CHECK(std::abs(std::abs(d[37]) - 512.0f) < 1.0f, "magnitude %f", std::abs(d[37]));
}

static void testRing()
{
    std::printf("[SpscRing] threaded integrity\n");
    dali::SpscRing<int> ring(1024);
    const int total = 2000000;
    std::thread prod([&] {
        int next = 0; int chunk[64];
        while (next < total)
        {
            int n = std::min(64, total - next);
            for (int i = 0; i < n; ++i) chunk[i] = next + i;
            next += int(ring.push(chunk, size_t(n)));
        }
    });
    int expect = 0; bool ok = true; int buf[128];
    while (expect < total)
    {
        size_t n = ring.pop(buf, 128);
        for (size_t i = 0; i < n; ++i) if (buf[i] != expect++) ok = false;
    }
    prod.join();
    CHECK(ok, "sequence corrupted");
    int tmp[8]; CHECK(ring.pop(tmp, 8) == 0, "ring not empty");
}

static void testTempo(double bpm)
{
    const double sr = 48000.0;
    dali::FeatureExtractor fx; fx.prepare(sr);
    Track t = makeTrack(bpm, 16.0, sr);
    std::vector<float> phaseAtKick;
    size_t nextKick = 0; double time = 0; const double hop = dali::FeatureExtractor::hopSize / sr;
    runExtractor(fx, t, [&](const dali::AudioFeatures& f) {
        time += hop;
        while (nextKick < t.kickTimes.size() && t.kickTimes[nextKick] <= time)
        {
            if (time > 10.0) { float ph = f.beatPhase; if (ph > 0.5f) ph -= 1.0f; phaseAtKick.push_back(ph); }
            ++nextKick;
        }
    });
    const auto& f = fx.features();
    double meanAbs = 0; for (float p : phaseAtKick) meanAbs += std::abs(p);
    meanAbs /= std::max<size_t>(1, phaseAtKick.size());
    const double beatsExpected = 16.0 * bpm / 60.0;
    std::printf("[Tempo %5.1f] detected %.2f BPM  conf %.2f  kicks %u/%.0f  mean |phase err| %.3f beat\n",
                bpm, f.bpm, f.bpmConfidence, f.kickCount, beatsExpected, meanAbs);
    CHECK(std::abs(f.bpm - bpm) < 1.5, "bpm %.2f vs %.1f", f.bpm, bpm);
    CHECK(f.kickCount > beatsExpected * 0.8 && f.kickCount < beatsExpected * 1.2, "kick count %u", f.kickCount);
    CHECK(meanAbs < 0.12, "phase error %.3f", meanAbs);
}

static void testLevelIndependence()
{
    std::printf("[AGC] level independence\n");
    const double sr = 44100.0;
    float bassLoud = 0, bassQuiet = 0;
    for (int pass = 0; pass < 2; ++pass)
    {
        dali::FeatureExtractor fx; fx.prepare(sr);
        Track t = makeTrack(140, 8.0, sr, pass == 0 ? 1.0f : 0.05f);   // -26 dB
        double acc = 0; int n = 0;
        runExtractor(fx, t, [&](const dali::AudioFeatures& f) { if (f.streamTime > 4.0) { acc += f.bassEnv; ++n; } });
        (pass == 0 ? bassLoud : bassQuiet) = float(acc / n);
    }
    std::printf("  mean bass env: loud %.3f  quiet(-26dB) %.3f\n", bassLoud, bassQuiet);
    CHECK(std::abs(bassLoud - bassQuiet) < 0.1f, "AGC mismatch");
    CHECK(bassLoud > 0.3f, "bass too low %.3f", bassLoud);
}

static void testSilenceStereoCentroid()
{
    std::printf("[Silence / Stereo / Centroid]\n");
    const double sr = 48000.0; const int H = dali::FeatureExtractor::hopSize;
    dali::FeatureExtractor fx; fx.prepare(sr);
    std::vector<float> z(H, 0.0f);
    for (int i = 0; i < 200; ++i) fx.processHop(z.data(), z.data());
    auto f = fx.features();
    CHECK(f.silent, "not silent");
    CHECK(f.bassEnv < 1e-3f && f.kick < 1e-3f && f.energy < 1e-3f, "silence produced energy");

    std::vector<float> L(H), R(H);
    for (int blk = 0; blk < 200; ++blk)
    {
        for (int i = 0; i < H; ++i) { L[size_t(i)] = 0.5f * float(std::sin(2 * PI * 200 * (blk * H + i) / sr)); R[size_t(i)] = 0.0f; }
        fx.processHop(L.data(), R.data());
    }
    f = fx.features();
    CHECK(f.pan < -0.9f, "hard-left pan %.2f", f.pan);
    CHECK(f.width > 0.9f, "hard-left width %.2f", f.width);
    const float cLow = f.centroid;
    for (int blk = 0; blk < 200; ++blk)
    {
        for (int i = 0; i < H; ++i) L[size_t(i)] = R[size_t(i)] = 0.5f * float(std::sin(2 * PI * 6000 * (blk * H + i) / sr));
        fx.processHop(L.data(), R.data());
    }
    f = fx.features();
    std::printf("  centroid 200Hz %.2f  6kHz %.2f  mono width %.2f\n", cLow, f.centroid, f.width);
    CHECK(f.centroid > cLow + 0.4f, "centroid ordering");
    CHECK(f.width < 0.05f, "mono width %.2f", f.width);
}

static void testModulation()
{
    std::printf("[Modulation]\n");
    dali::ModulationEngine eng; dali::ModSlotArray slots {}; dali::ModSourceValues src {};
    for (auto& s : slots) s.enabled = false;
    auto& s = slots[0]; s = {}; s.source = int(dali::ModSource::Bass); s.target = 2; s.amount = 0.5f;
    s.attackMs = 0; s.releaseMs = 0; s.smoothingMs = 0;
    float off[4];
    src[dali::ModSource::Bass] = 1.0f;
    eng.process(slots, src, 1 / 60.0f, off, 4);
    CHECK(std::abs(off[2] - 0.5f) < 1e-5f && off[0] == 0.0f, "basic %.3f", off[2]);
    s.invert = true; eng.process(slots, src, 1 / 60.0f, off, 4);
    CHECK(std::abs(off[2]) < 1e-5f, "invert %.3f", off[2]);
    s.invert = false; s.bipolar = true; src[dali::ModSource::Bass] = 0.0f; eng.process(slots, src, 1 / 60.0f, off, 4);
    CHECK(std::abs(off[2] + 0.5f) < 1e-5f, "bipolar %.3f", off[2]);
    s.bipolar = false; s.min = 0.2f; s.max = 0.6f; src[dali::ModSource::Bass] = 1.0f; eng.process(slots, src, 1 / 60.0f, off, 4);
    CHECK(std::abs(off[2] - 0.3f) < 1e-5f, "min/max %.3f", off[2]);
    CHECK(dali::ModulationEngine::shapeCurve(0.25f, 1.0f) > 0.6f && dali::ModulationEngine::shapeCurve(0.25f, -1.0f) < 0.01f, "curve");
    s.min = 0; s.max = 1; s.sensitivity = 2.0f; src[dali::ModSource::Bass] = 0.3f; eng.process(slots, src, 1 / 60.0f, off, 4);
    CHECK(std::abs(off[2] - 0.3f) < 1e-5f, "sensitivity %.3f", off[2]);
    // attack: 100 ms, after 100 ms ≈ 63 %
    s.sensitivity = 1; s.attackMs = 100; eng.reset(); src[dali::ModSource::Bass] = 1.0f;
    for (int i = 0; i < 6; ++i) eng.process(slots, src, 1 / 60.0f, off, 4);
    CHECK(off[2] > 0.25f && off[2] < 0.38f, "attack envelope %.3f", off[2]);
    // two slots on the same target add up
    slots[1] = slots[0]; slots[1].attackMs = 0; slots[0].attackMs = 0; eng.process(slots, src, 1 / 60.0f, off, 4);
    CHECK(std::abs(off[2] - 1.0f) < 1e-4f, "sum %.3f", off[2]);
}

static void testClock()
{
    std::printf("[MusicalClock]\n");
    dali::MusicalClock c; dali::HostTiming host; dali::DetectedTiming det;
    for (int i = 0; i < 120; ++i) c.update(i / 60.0, 1 / 60.0, dali::SyncSource::Internal, host, det, 120.0, 2);
    CHECK(std::abs(c.beatClock() - 4.0) < 0.05, "internal beats %.3f", c.beatClock());
    host.valid = true; host.playing = true; host.bpm = 150; host.ppq = 33.5; host.barStartPpq = 32; host.stamp = 2.0;
    c.update(2.0, 1 / 60.0, dali::SyncSource::Auto, host, det, 120.0, 2);
    CHECK(c.source() == dali::MusicalClock::ActiveSource::Host, "host not selected");
    CHECK(std::abs(c.beatClock() - 33.5) < 1e-6, "host lock %.3f", c.beatClock());
    CHECK(std::abs(c.barPhase() - 0.375f) < 1e-4f, "bar phase %.3f", c.barPhase());
    c.update(2.1, 0.1, dali::SyncSource::Auto, host, det, 120.0, 2);
    CHECK(std::abs(c.beatClock() - 33.75) < 0.01, "host extrapolation %.3f", c.beatClock());
    host.playing = false; det.bpm = 132; det.confidence = 0.6; det.beatPhase = 0.0; det.stamp = 3.0;
    for (int i = 0; i < 240; ++i) c.update(3.0 + i / 60.0, 1 / 60.0, dali::SyncSource::Auto, host, det, 120.0, 4);
    CHECK(c.source() == dali::MusicalClock::ActiveSource::Detect, "detect not selected");
    const double expectPhase = std::fmod(239 / 60.0 * 132 / 60.0, 1.0);
    double err = c.beatPhase() - expectPhase; err -= std::round(err);
    CHECK(std::abs(err) < 0.05, "detect phase lock err %.3f", err);
    CHECK(std::abs(dali::syncDivisionBeats(8) - 2.0 / 3.0) < 1e-9, "triplet division");
}

static void testImageDNA()
{
    std::printf("[ImageDNA]\n");
    const int W = 300, H = 200;
    std::vector<unsigned char> img(size_t(W) * H * 4);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            unsigned char* p = &img[(size_t(y) * W + x) * 4];
            const bool in = std::hypot(x - 150.0, y - 100.0) < 60.0;
            p[0] = in ? 200 : 10; p[1] = in ? 40 : 10; p[2] = in ? 220 : 12; p[3] = 255;
        }
    auto t0 = std::chrono::steady_clock::now();
    auto dna = dali::ImageDNA::analyse(img.data(), W, H, 1024);
    auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("  %dx%d aspect %.2f coverage %.3f  palette0 %.2f %.2f %.2f  (%.1f ms)\n", dna.width, dna.height,
                dna.aspect, dna.coverage, dna.palette[0][0], dna.palette[0][1], dna.palette[0][2], ms);
    CHECK(dna.isValid(), "invalid");
    CHECK(std::abs(dna.aspect - 1.5f) < 0.01f, "aspect");
    const float expectCov = float(PI * 60 * 60 / (W * H));
    CHECK(std::abs(dna.coverage - expectCov) < 0.03f, "coverage %.3f vs %.3f", dna.coverage, expectCov);
    const float* centre = &dna.dna[(size_t(100) * W + 150) * 4];
    const float* edge   = &dna.dna[(size_t(100) * W + 210) * 4];
    CHECK(centre[3] > 0.9f && centre[1] < 0.2f, "centre presence/edge %.2f %.2f", centre[3], centre[1]);
    CHECK(edge[1] > 0.5f, "edge strength %.2f", edge[1]);
    CHECK(centre[2] > edge[2], "distance field ordering");
    bool purple = false;
    for (auto& c : dna.palette) if (c[0] > 0.6f && c[2] > 0.6f && c[1] < 0.3f) purple = true;
    CHECK(purple, "dominant colour not extracted");
    auto big = std::vector<unsigned char>(size_t(4000) * 3000 * 4, 128);
    t0 = std::chrono::steady_clock::now();
    auto d2 = dali::ImageDNA::analyse(big.data(), 4000, 3000, 1024);
    ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("  4000x3000 → %dx%d in %.0f ms (runs on a background thread)\n", d2.width, d2.height, ms);
    CHECK(d2.width == 1024 && d2.height == 768, "resample size");
}

static void testExtractorCost()
{
    const double sr = 48000.0;
    dali::FeatureExtractor fx; fx.prepare(sr);
    Track t = makeTrack(140, 10.0, sr);
    auto t0 = std::chrono::steady_clock::now();
    runExtractor(fx, t);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    const double hops = double(t.L.size() / dali::FeatureExtractor::hopSize);
    std::printf("[Cost] analysis %.1f µs per hop  (%.2f%% of one core in real time)\n",
                ms * 1000.0 / hops, ms / 10000.0 * 100.0);
}

// ---------------------------------------------------------------------------
//  v2: snare / hi-hat detection, build & drop, activity gate
// ---------------------------------------------------------------------------
struct DrumTrack { std::vector<float> L, R; int snares = 0, hats = 0, kicks = 0; };

// kick on 1 & 3, snare on 2 & 4, closed hat on every 8th.
static void addDrums(DrumTrack& t, double bpm, double t0, double t1, double sr, bool kick, bool snare, bool hats,
                     bool riser, unsigned seed)
{
    std::mt19937 rng(seed); std::uniform_real_distribution<float> U(-1.0f, 1.0f);
    const double beat = 60.0 / bpm;
    const size_t a = size_t(t0 * sr), b = std::min(t.L.size(), size_t(t1 * sr));
    double kp = 0; float pn = 0, lp1 = 0, lp2 = 0; int lastBeat = -1, lastEighth = -1;
    for (size_t i = a; i < b; ++i)
    {
        const double time = i / sr - t0;
        const int bi = int(time / beat), ei = int(time / (beat * 0.5));
        const double bpos = time - bi * beat, epos = time - ei * beat * 0.5;
        const bool kickBeat = (bi % 2) == 0;
        if (bi != lastBeat) { lastBeat = bi; if (kick && kickBeat) ++t.kicks; if (snare && !kickBeat) ++t.snares; }
        if (ei != lastEighth) { lastEighth = ei; if (hats) ++t.hats; }
        float m = 0;
        if (kick && kickBeat)
        {
            kp += 2 * PI * (48.0 + 102.0 * std::exp(-bpos * 30.0)) / sr;
            m += float(std::sin(kp) * std::exp(-bpos * 9.0)) * 0.8f;
        }
        const float nz = U(rng);
        if (snare && !kickBeat)                                  // band-passed noise + 190 Hz body
        {
            lp1 += 0.35f * (nz - lp1); lp2 += 0.04f * (lp1 - lp2);
            const float bp = (lp1 - lp2) * 2.2f;
            m += (bp + 0.3f * float(std::sin(2 * PI * 190.0 * bpos))) * float(std::exp(-bpos * 22.0)) * 0.45f;
        }
        if (hats) { const float hp = nz - pn; m += hp * float(std::exp(-epos * 70.0)) * 0.12f; }
        pn = nz;
        if (riser)
        {
            const double p = time / std::max(1e-9, t1 - t0);
            m += U(rng) * 0.02f * float(p) + 0.03f * float(std::sin(2 * PI * (300 + 900 * p) * time));
        }
        m += U(rng) * 0.004f;                                    // room tone: never digital silence
        t.L[i] += m; t.R[i] += m;
    }
}

static void testDrumHits()
{
    const double sr = 48000, secs = 24, bpm = 124;
    DrumTrack t; t.L.assign(size_t(secs * sr), 0.0f); t.R = t.L;
    addDrums(t, bpm, 0, secs, sr, true, true, true, false, 7);
    dali::FeatureExtractor fx; fx.prepare(sr);
    runExtractor(fx, Track { t.L, t.R, {} });
    const auto& f = fx.features();
    const double sn = double(f.snareCount) / t.snares, hh = double(f.hatCount) / t.hats, kk = double(f.kickCount) / t.kicks;
    std::printf("[Drums] kicks %u/%d  snares %u/%d  hats %u/%d\n", f.kickCount, t.kicks, f.snareCount, t.snares, f.hatCount, t.hats);
    CHECK(sn > 0.8 && sn < 1.15, "snare detection ratio %.2f", sn);
    CHECK(hh > 0.7 && hh < 1.2, "hat detection ratio %.2f", hh);
    CHECK(kk > 0.8 && kk < 1.15, "kick detection ratio with snares present %.2f", kk);

    auto plain = makeTrack(128, 20, sr);      // four-on-the-floor + off-beat hats, no snare
    dali::FeatureExtractor fx2; fx2.prepare(sr);
    runExtractor(fx2, plain);
    const double beats = 20.0 * 128 / 60;
    std::printf("[Drums] snare false positives on kick+hat track: %u of %.0f beats\n", fx2.features().snareCount, beats);
    CHECK(fx2.features().snareCount < beats * 0.15, "snare false positives %u", fx2.features().snareCount);
}

static void testBuildDrop()
{
    // 0-16 s groove, 16-26 s breakdown (hats + riser, no kick), 26-36 s groove again
    const double sr = 48000, bpm = 128;
    DrumTrack t; t.L.assign(size_t(36 * sr), 0.0f); t.R = t.L;
    addDrums(t, bpm, 0, 16, sr, true, true, true, false, 1);
    addDrums(t, bpm, 16, 26, sr, false, false, true, true, 2);
    addDrums(t, bpm, 26, 36, sr, true, true, true, false, 3);
    dali::FeatureExtractor fx; fx.prepare(sr);
    double dropAt = -1, time = 0; float buildGroove = 0, buildLate = 0, activityMin = 1;
    std::uint32_t drops = 0;
    runExtractor(fx, Track { t.L, t.R, {} }, [&](const dali::AudioFeatures& f)
    {
        time += dali::FeatureExtractor::hopSize / sr;
        if (f.dropCount != drops) { drops = f.dropCount; if (dropAt < 0) dropAt = time; }
        if (time > 4 && time < 15) buildGroove = std::max(buildGroove, f.build);
        if (time > 23 && time < 26) buildLate = std::max(buildLate, f.build);
        if (time > 2) activityMin = std::min(activityMin, f.activity);
    });
    std::printf("[Structure] drops %u (first at %.2f s)  build: groove %.2f, end of breakdown %.2f\n", drops, dropAt, buildGroove, buildLate);
    CHECK(drops == 1, "expected exactly one drop, got %u", drops);
    CHECK(dropAt > 25.9 && dropAt < 27.0, "drop time %.2f", dropAt);
    CHECK(buildGroove < 0.15f, "build during groove %.2f", buildGroove);
    CHECK(buildLate > 0.5f, "build at end of breakdown %.2f", buildLate);
    CHECK(activityMin > 0.9f, "activity stays up while music plays (%.2f)", activityMin);

    std::vector<float> z(size_t(3 * sr), 0.0f);  // silence → activity falls to 0
    runExtractor(fx, Track { z, z, {} });
    CHECK(fx.features().activity < 0.05f, "activity after silence %.3f", fx.features().activity);
    CHECK(fx.features().build < 0.05f, "build after silence %.3f", fx.features().build);
}

static void testSpectrumWave()
{
    const double sr = 48000;
    for (double hz : { 100.0, 1000.0, 6000.0 })
    {
        std::vector<float> s(size_t(sr * 1.5));
        for (size_t i = 0; i < s.size(); ++i) s[i] = 0.3f * float(std::sin(2 * PI * hz * i / sr));
        dali::FeatureExtractor fx; fx.prepare(sr);
        runExtractor(fx, Track { s, s, {} });
        const auto& f = fx.features();
        int arg = 0; for (int i = 1; i < dali::AudioFeatures::kSpectrumBands; ++i) if (f.spectrum[size_t(i)] > f.spectrum[size_t(arg)]) arg = i;
        const double expect = dali::AudioFeatures::kSpectrumBands * std::log(hz / 30.0) / std::log(16000.0 / 30.0);
        float far = 0; for (int i = 0; i < dali::AudioFeatures::kSpectrumBands; ++i) if (std::abs(i - expect) > 20) far = std::max(far, f.spectrum[size_t(i)]);
        float wmax = 0, wmean = 0; for (float w : f.wave) { wmax = std::max(wmax, std::abs(w)); wmean += w; }
        wmean /= float(f.wave.size());
        std::printf("[Spectrum] %5.0f Hz -> band %3d (expected %.1f)  peak %.2f  far-away max %.2f  wave |max| %.2f mean %+.2f\n",
                    hz, arg, expect, f.spectrum[size_t(arg)], far, wmax, wmean);
        CHECK(std::abs(arg - expect) <= 2.0, "%.0f Hz sine peaks in band %d, expected %.1f", hz, arg, expect);
        CHECK(f.spectrum[size_t(arg)] > 0.8f, "peak level %.2f", f.spectrum[size_t(arg)]);
        CHECK(far < 0.35f, "energy far from the tone %.2f", far);
        CHECK(wmax > 0.85f && std::abs(wmean) < 0.25f, "waveform max %.2f mean %.2f", wmax, wmean);
    }
    // silence → spectrum and waveform fall to zero
    dali::FeatureExtractor fx; fx.prepare(sr);
    auto t = makeTrack(128, 4, sr);
    runExtractor(fx, t);
    std::vector<float> z(size_t(sr * 2), 0.0f);
    runExtractor(fx, Track { z, z, {} });
    float mx = 0; for (float v : fx.features().spectrum) mx = std::max(mx, v);
    CHECK(mx < 0.02f, "spectrum after silence %.3f", mx);
}

int main()
{
    testFFT();
    testRing();
    for (double b : { 100.0, 128.0, 140.0, 145.0, 150.0, 174.0 }) testTempo(b);
    testLevelIndependence();
    testSilenceStereoCentroid();
    testModulation();
    testClock();
    testDrumHits();
    testBuildDrop();
    testSpectrumWave();
    testImageDNA();
    testExtractorCost();
    std::printf("\n%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
