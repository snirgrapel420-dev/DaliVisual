// ============================================================================
//  Factory presets — defined in code so they always match the parameter set.
//  Each preset lists only what differs from the defaults (the processor
//  resets every unspecified parameter to its default when loading).
// ============================================================================
#include "PresetManager.h"
#include "../Core/Parameters.h"
#include "../Image/TemplateGenerator.h"
#include "../Modulation/ModulationMatrix.h"
#include "../Render/EffectChain.h"

namespace dali
{
namespace
{
struct Route { ModSource src; const char* target; float amount; float attack = 5.0f, release = 150.0f; bool bipolar = false; };

struct Def
{
    const char* name;
    std::vector<std::pair<juce::String, float>> values;
    std::vector<Route> routes;
    bool templateRoutes = false;
};

juce::ValueTree makeState(const Def& d)
{
    juce::ValueTree st("DaliVisualState");
    juce::ValueTree p("Params");
    for (auto& [id, v] : d.values) p.setProperty(id, v, nullptr);
    st.appendChild(p, nullptr);

    ModulationMatrix m;
    int slot = 0;
    for (auto& r : d.routes)
    {
        const int t = ModulationTarget::fromParamId(r.target);
        jassert(t >= 0);
        if (t < 0 || slot >= kMaxModSlots) continue;
        ModSlot s; s.source = int(r.src); s.target = t; s.amount = r.amount;
        s.attackMs = r.attack; s.releaseMs = r.release; s.bipolar = r.bipolar;
        m.setSlot(slot++, s);
    }
    if (d.templateRoutes) TemplateGenerator::addReactiveRoutes(m);
    st.appendChild(m.toValueTree(), nullptr);
    st.appendChild(EffectChain().toValueTree(), nullptr);
    return st;
}

juce::String on(const char* fx)  { return params::id::fxOn(fx); }
juce::String amt(const char* fx) { return params::id::fxAmt(fx); }
juce::String p2(const char* fx)  { return params::id::fxP2(fx); }
}

std::vector<Preset> createFactoryPresets()
{
    using namespace params::id;
    const std::vector<Def> defs = {
        { "00 Init", {}, { { ModSource::Bass, "macroA", 0.25f } } },

        { "01 Kinetic Kaleido - Goa Pulse",
          { { scene, 0 }, { palette, 7 }, { macroA, 0.55f }, { macroB, 0.6f }, { macroC, 0.7f }, { macroD, 0.55f },
            { on("glow"), 1 }, { amt("glow"), 0.35f }, { on("chromatic"), 1 }, { amt("chromatic"), 0.15f } },
          { { ModSource::Kick, "intensity", 0.2f, 0, 120 }, { ModSource::Bass, "macroB", 0.2f },
            { ModSource::Snare, "fx_chromatic_amt", 0.6f, 0, 200 }, { ModSource::HiHat, "fx_glow_amt", 0.25f, 0, 80 },
            { ModSource::Drop, "macroD", 0.4f, 0, 400 } } },

        { "02 Organic Flux - Deep Breath",
          { { scene, 1 }, { palette, 2 }, { macroA, 0.45f }, { macroD, 0.35f },
            { on("vignette"), 1 }, { amt("vignette"), 0.5f }, { on("glow"), 1 }, { amt("glow"), 0.25f } },
          { { ModSource::Bass, "macroA", 0.3f, 10, 300 }, { ModSource::Mid, "macroC", 0.2f },
            { ModSource::Centroid, "colorShift", 0.2f, 50, 500 } } },

        { "03 Infinite Tunnel - Hyperdrive",
          { { scene, 2 }, { palette, 4 }, { macroA, 0.3f }, { macroB, 0.6f }, { macroC, 0.65f }, { macroD, 0.6f },
            { on("trails"), 1 }, { amt("trails"), 0.55f }, { on("rgbsplit"), 1 }, { amt("rgbsplit"), 0.1f } },
          { { ModSource::Energy, "macroC", 0.25f, 50, 400 }, { ModSource::Snare, "fx_rgbsplit_amt", 0.5f, 0, 180 },
            { ModSource::Kick, "macroA", 0.2f, 0, 150 }, { ModSource::Build, "macroC", -0.35f, 200, 300 } } },

        { "04 Fractal Temple - Sacred Geometry",
          { { scene, 3 }, { palette, 2 }, { macroA, 0.4f }, { macroB, 0.6f }, { macroD, 0.45f }, { syncDiv, 2 },
            { on("glow"), 1 }, { amt("glow"), 0.4f }, { on("vignette"), 1 }, { amt("vignette"), 0.4f } },
          { { ModSource::Kick, "macroD", 0.3f, 0, 200 }, { ModSource::Bass, "macroC", 0.1f },
            { ModSource::SyncLFO, "hue", 0.05f, 0, 0 } } },

        { "05 Acid Matrix - 303 Squelch",
          { { scene, 4 }, { palette, 1 }, { macroA, 0.5f }, { macroB, 0.7f }, { macroC, 0.8f }, { macroD, 0.6f },
            { on("rgbsplit"), 1 }, { amt("rgbsplit"), 0.12f }, { on("noise"), 1 }, { amt("noise"), 0.12f } },
          { { ModSource::Centroid, "macroB", 0.35f, 20, 200 }, { ModSource::Kick, "fx_rgbsplit_amt", 0.5f, 0, 120 },
            { ModSource::Beat, "brightness", 0.12f, 0, 150 }, { ModSource::HiHat, "fx_noise_amt", 0.3f, 0, 60 } } },

        { "06 Liquid Dream - Mercury",
          { { scene, 5 }, { palette, 5 }, { macroB, 0.55f }, { macroD, 0.45f },
            { on("glow"), 1 }, { amt("glow"), 0.3f }, { on("warp"), 1 }, { amt("warp"), 0.12f } },
          { { ModSource::Bass, "macroB", 0.3f, 10, 250 }, { ModSource::High, "fx_glow_amt", 0.3f } } },

        { "07 Neural Bloom - Synapse Fire",
          { { scene, 6 }, { palette, 2 }, { macroA, 0.45f }, { macroB, 0.6f },
            { on("glow"), 1 }, { amt("glow"), 0.55f }, { p2("glow"), 0.2f }, { on("trails"), 1 }, { amt("trails"), 0.45f } },
          { { ModSource::Snare, "macroD", 0.5f, 0, 250 }, { ModSource::Bass, "macroC", 0.3f },
            { ModSource::HiHat, "fx_glow_amt", 0.2f, 0, 60 } } },

        { "08 Psychedelic Void - Event Horizon",
          { { scene, 7 }, { palette, 7 }, { macroA, 0.6f }, { macroB, 0.7f }, { macroD, 0.5f },
            { on("feedback"), 1 }, { amt("feedback"), 0.55f }, { on("vignette"), 1 }, { amt("vignette"), 0.6f } },
          { { ModSource::Kick, "macroD", 0.4f, 0, 200 }, { ModSource::Energy, "macroC", 0.3f, 50, 400 } } },

        { "09 Image Reactor - Kaleidoscope",
          { { scene, 16 }, { imgMode, 0 }, { palette, 7 }, { bloom, 0.25f }, { "tplSymCount", 8 }, { "tplColorExtract", 0.15f },
            { "tplFeedback", 0.35f }, { dynamics, 0.6f } },
          {}, true },

        { "10 Image Reactor - Liquid Dream",
          { { scene, 16 }, { imgMode, 1 }, { palette, 2 }, { bloom, 0.25f }, { "tplWarp", 0.45f }, { "tplDistortion", 0.35f },
            { "tplColorExtract", 0.0f }, { "tplFeedback", 0.3f } },
          {}, true },

        { "11 Mono - Silver Kaleido",
          { { scene, 0 }, { palette, 0 }, { macroA, 0.35f }, { macroB, 0.75f }, { macroD, 0.3f },
            { on("glow"), 1 }, { amt("glow"), 0.3f }, { on("noise"), 1 }, { amt("noise"), 0.1f } },
          { { ModSource::Kick, "intensity", 0.25f, 0, 150 } } },

        { "13 Festival - Build and Drop",
          { { scene, 0 }, { palette, 7 }, { dynamics, 0.95f }, { audioDrive, 0.85f }, { macroA, 0.6f }, { macroB, 0.65f },
            { on("glow"), 1 }, { amt("glow"), 0.45f }, { on("rgbsplit"), 1 }, { amt("rgbsplit"), 0.08f },
            { on("feedback"), 1 }, { amt("feedback"), 0.35f } },
          { { ModSource::Kick, "intensity", 0.25f, 0, 120 }, { ModSource::Snare, "fx_rgbsplit_amt", 0.5f, 0, 160 },
            { ModSource::Build, "macroB", 0.4f, 300, 200 }, { ModSource::Build, "fx_feedback_amt", 0.4f, 300, 150 },
            { ModSource::Drop, "fx_glow_amt", 0.5f, 0, 900 }, { ModSource::HiHat, "fx_noise_amt", 0.15f, 0, 50 } } },

        { "14 Auto Pilot - Journey",
          { { scene, 3 }, { palette, 2 }, { autoPilot, 2 }, { autoBars, 2 }, { autoOnDrop, 1 }, { dynamics, 0.75f },
            { on("glow"), 1 }, { amt("glow"), 0.35f }, { on("vignette"), 1 }, { amt("vignette"), 0.45f } },
          { { ModSource::Kick, "intensity", 0.2f, 0, 120 }, { ModSource::Snare, "macroD", 0.3f, 0, 200 },
            { ModSource::Bass, "macroA", 0.2f, 10, 250 }, { ModSource::Centroid, "colorShift", 0.15f, 60, 500 } } },

        // ---------------- v3: sound-driven psychedelic scenes ----------------
        { "20 Kali Cathedral - Sacred Flight",
          { { scene, 8 }, { palette, 7 }, { macroA, 0.45f }, { macroB, 0.6f }, { macroC, 0.45f }, { macroD, 0.55f },
            { bloom, 0.55f }, { dynamics, 0.8f }, { on("chromatic"), 1 }, { amt("chromatic"), 0.12f } },
          { { ModSource::Snare, "macroA", 0.18f, 0, 220 }, { ModSource::Kick, "macroD", 0.35f, 0, 160 },
            { ModSource::Build, "macroC", -0.3f, 300, 200 }, { ModSource::Drop, "bloom", 0.4f, 0, 900 } } },
        { "21 Kali Cathedral - Gold Temple",
          { { scene, 8 }, { palette, 6 }, { macroA, 0.25f }, { macroB, 0.85f }, { macroC, 0.3f }, { macroD, 0.7f },
            { bloom, 0.7f }, { on("vignette"), 1 }, { amt("vignette"), 0.5f } },
          { { ModSource::Kick, "intensity", 0.2f, 0, 150 }, { ModSource::Centroid, "colorShift", 0.2f, 60, 500 },
            { ModSource::HiHat, "macroD", 0.2f, 0, 60 } } },

        { "22 Spectral Mandala - Neon Lotus",
          { { scene, 9 }, { palette, 7 }, { macroA, 0.35f }, { macroB, 0.6f }, { macroC, 0.4f }, { macroD, 0.6f },
            { bloom, 0.6f }, { dynamics, 0.7f } },
          { { ModSource::Snare, "macroC", 0.25f, 0, 200 }, { ModSource::Drop, "macroA", 0.3f, 0, 1200 } } },
        { "23 Spectral Mandala - Ice Crystal",
          { { scene, 9 }, { palette, 5 }, { macroA, 0.75f }, { macroB, 0.4f }, { macroC, 0.2f }, { macroD, 0.35f },
            { bloom, 0.5f }, { on("rgbsplit"), 1 }, { amt("rgbsplit"), 0.05f } },
          { { ModSource::HiHat, "fx_rgbsplit_amt", 0.35f, 0, 60 }, { ModSource::Bass, "macroB", 0.25f, 10, 200 } } },

        { "24 Julia Bloom - Rainbow Spiral",
          { { scene, 10 }, { palette, 7 }, { macroA, 0.4f }, { macroB, 0.6f }, { macroC, 0.45f }, { macroD, 0.5f },
            { bloom, 0.5f }, { dynamics, 0.7f } },
          { { ModSource::Snare, "macroC", 0.2f, 0, 250 }, { ModSource::Build, "macroB", 0.3f, 400, 300 } } },
        { "25 Julia Bloom - Kaleido Flower",
          { { scene, 10 }, { palette, 2 }, { macroA, 0.82f }, { macroB, 0.7f }, { macroC, 0.3f }, { macroD, 0.6f },
            { bloom, 0.55f }, { on("glow"), 1 }, { amt("glow"), 0.25f } },
          { { ModSource::Kick, "macroD", 0.3f, 0, 180 }, { ModSource::Centroid, "colorShift", 0.25f, 60, 500 } } },

        { "26 Hyperspace - Warp Drive",
          { { scene, 11 }, { palette, 4 }, { macroA, 0.45f }, { macroB, 0.6f }, { macroC, 0.55f }, { macroD, 0.5f },
            { bloom, 0.6f }, { dynamics, 0.85f }, { on("trails"), 1 }, { amt("trails"), 0.3f } },
          { { ModSource::Snare, "macroA", 0.2f, 0, 200 }, { ModSource::Build, "macroC", 0.35f, 400, 150 },
            { ModSource::Drop, "fx_trails_amt", 0.4f, 0, 900 } } },
        { "27 Hyperspace - Acid Wormhole",
          { { scene, 11 }, { palette, 1 }, { macroA, 0.7f }, { macroB, 0.8f }, { macroC, 0.4f }, { macroD, 0.7f },
            { bloom, 0.5f }, { on("chromatic"), 1 }, { amt("chromatic"), 0.15f } },
          { { ModSource::Kick, "fx_chromatic_amt", 0.4f, 0, 150 }, { ModSource::HiHat, "macroB", 0.15f, 0, 60 } } },

        { "28 Iridescent Oil - Liquid Chrome",
          { { scene, 12 }, { palette, 0 }, { macroA, 0.5f }, { macroB, 0.5f }, { macroC, 0.5f }, { macroD, 0.7f },
            { bloom, 0.45f } },
          { { ModSource::Bass, "macroC", 0.25f, 20, 300 }, { ModSource::HiHat, "macroD", 0.2f, 0, 60 } } },
        { "29 Iridescent Oil - Psychedelic Film",
          { { scene, 12 }, { palette, 7 }, { macroA, 0.4f }, { macroB, 0.8f }, { macroC, 0.65f }, { macroD, 0.5f },
            { bloom, 0.5f }, { dynamics, 0.7f }, { on("kaleido"), 1 }, { amt("kaleido"), 0.6f } },
          { { ModSource::Snare, "macroB", 0.2f, 0, 250 }, { ModSource::SyncLFO, "colorShift", 0.1f, 0, 0 } } },

        { "30 Hyperbolic Dream - Circle Limit",
          { { scene, 13 }, { palette, 2 }, { macroA, 0.1f }, { macroB, 0.35f }, { macroC, 0.4f }, { macroD, 0.5f },
            { bloom, 0.55f } },
          { { ModSource::Kick, "macroB", 0.2f, 0, 150 }, { ModSource::Drop, "macroA", 0.25f, 0, 50 } } },
        { "31 Hyperbolic Dream - Escher Rainbow",
          { { scene, 13 }, { palette, 7 }, { macroA, 0.45f }, { macroB, 0.3f }, { macroC, 0.55f }, { macroD, 0.6f },
            { bloom, 0.5f }, { dynamics, 0.7f } },
          { { ModSource::Snare, "colorShift", 0.12f, 0, 300 }, { ModSource::Centroid, "macroD", 0.2f, 60, 400 } } },

        { "32 Infinite Feedback - Tunnel of Light",
          { { scene, 14 }, { palette, 7 }, { macroA, 0.45f }, { macroB, 0.5f }, { macroC, 0.45f }, { macroD, 0.7f },
            { bloom, 0.55f }, { dynamics, 0.75f } },
          { { ModSource::Snare, "macroC", 0.25f, 0, 200 }, { ModSource::Build, "macroD", 0.25f, 400, 200 } } },
        { "33 Infinite Feedback - Crystal Kaleidoscope",
          { { scene, 14 }, { palette, 5 }, { macroA, 0.8f }, { macroB, 0.35f }, { macroC, 0.25f }, { macroD, 0.8f },
            { bloom, 0.5f } },
          { { ModSource::Kick, "macroB", 0.25f, 0, 150 }, { ModSource::HiHat, "colorShift", 0.06f, 0, 80 } } },

        { "34 Waveform Geometry - Sacred Scope",
          { { scene, 15 }, { palette, 7 }, { macroA, 0.6f }, { macroB, 0.6f }, { macroC, 0.35f }, { macroD, 0.55f },
            { bloom, 0.6f } },
          { { ModSource::Snare, "macroC", 0.2f, 0, 200 }, { ModSource::Drop, "macroA", 0.3f, 0, 1500 } } },
        { "35 Waveform Geometry - Laser Temple",
          { { scene, 15 }, { palette, 3 }, { macroA, 0.9f }, { macroB, 0.8f }, { macroC, 0.5f }, { macroD, 0.75f },
            { bloom, 0.7f }, { on("feedback"), 1 }, { amt("feedback"), 0.3f } },
          { { ModSource::Kick, "bloom", 0.25f, 0, 150 }, { ModSource::HiHat, "macroB", 0.15f, 0, 50 } } },

        { "36 Auto Pilot - Psychedelic Journey",
          { { scene, 8 }, { palette, 7 }, { autoPilot, 2 }, { autoBars, 2 }, { autoOnDrop, 1 }, { dynamics, 0.8f },
            { bloom, 0.55f } },
          { { ModSource::Kick, "intensity", 0.2f, 0, 120 }, { ModSource::Snare, "macroD", 0.25f, 0, 200 },
            { ModSource::Build, "macroC", 0.2f, 400, 200 } } },

        // ---------------- Image Reactor: the dropped image is the visual ----------------
        { "37 Image Reactor - Tunnel Ride",
          { { scene, 16 }, { imgMode, 2 }, { palette, 7 }, { bloom, 0.3f }, { "tplSymCount", 6 }, { "tplTwist", 0.25f },
            { "tplColorExtract", 0.1f }, { "tplFeedback", 0.25f }, { dynamics, 0.75f } },
          { { ModSource::Snare, "tplTwist", 0.2f, 0, 200 }, { ModSource::Kick, "macroC", 0.15f, 0, 150 } } },
        { "38 Image Reactor - Spectral Slices",
          { { scene, 16 }, { imgMode, 3 }, { palette, 7 }, { bloom, 0.2f }, { "tplDetail", 0.55f }, { "tplWarp", 0.35f },
            { "tplColorExtract", 0.0f }, { "tplFeedback", 0.2f } },
          { { ModSource::HiHat, "tplDistortion", 0.3f, 0, 60 } } },
        { "39 Image Reactor - Droste Infinity",
          { { scene, 16 }, { imgMode, 4 }, { palette, 2 }, { bloom, 0.3f }, { "tplDepth", 0.4f }, { "tplTwist", 0.3f },
            { "tplColorExtract", 0.2f }, { "tplFeedback", 0.3f }, { dynamics, 0.7f } },
          { { ModSource::Build, "tplTwist", 0.4f, 400, 200 }, { ModSource::Drop, "tplColorExtract", 0.6f, 0, 1500 } } },
        { "40 Image Reactor - Glitch Pressure",
          { { scene, 16 }, { imgMode, 5 }, { palette, 3 }, { bloom, 0.2f }, { "tplWarp", 0.45f }, { "tplDetail", 0.5f },
            { "tplDistortion", 0.45f }, { "tplColorExtract", 0.0f }, { "tplFeedback", 0.1f } },
          { { ModSource::Snare, "tplWarp", 0.3f, 0, 150 } } },
        { "41 Image Reactor - Depth 3D",
          { { scene, 16 }, { imgMode, 6 }, { palette, 4 }, { bloom, 0.3f }, { "tplDepth", 0.6f }, { "tplColorExtract", 0.1f },
            { "tplFeedback", 0.2f } },
          { { ModSource::Bass, "tplDepth", 0.25f, 10, 250 } } },
        { "42 Image Reactor - Neon Outline",
          { { scene, 16 }, { imgMode, 7 }, { palette, 7 }, { bloom, 0.55f }, { "tplSymCount", 6 }, { "tplEdge", 0.6f },
            { "tplFeedback", 0.45f }, { dynamics, 0.8f } },
          { { ModSource::Snare, "tplTwist", 0.2f, 0, 250 } } },

        { "12 Red Black - Ritual",
          { { scene, 3 }, { palette, 3 }, { macroA, 0.7f }, { macroB, 0.8f }, { macroC, 0.3f },
            { on("contrast"), 1 }, { amt("contrast"), 0.65f }, { on("vignette"), 1 }, { amt("vignette"), 0.7f } },
          { { ModSource::Kick, "macroD", 0.35f, 0, 150 }, { ModSource::SyncLFO, "colorShift", 0.08f, 0, 0 } } },
    };

    std::vector<Preset> out;
    for (auto& d : defs) out.push_back({ d.name, makeState(d) });
    return out;
}
} // namespace dali
