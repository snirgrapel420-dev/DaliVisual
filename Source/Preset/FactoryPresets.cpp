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

using namespace params::id;
juce::String on(const char* fx)  { return fxOn(fx); }
juce::String amt(const char* fx) { return fxAmt(fx); }
juce::String p2(const char* fx)  { return fxP2(fx); }
}

std::vector<Preset> createFactoryPresets()
{
    const std::vector<Def> defs = {
        { "00 Init", {}, { { ModSource::Bass, "macroA", 0.25f } } },

        { "01 Kinetic Kaleido - Goa Pulse",
          { { scene, 0 }, { palette, 7 }, { macroA, 0.55f }, { macroB, 0.6f }, { macroC, 0.7f }, { macroD, 0.55f },
            { on("glow"), 1 }, { amt("glow"), 0.35f }, { on("chromatic"), 1 }, { amt("chromatic"), 0.15f } },
          { { ModSource::Kick, "intensity", 0.2f, 0, 120 }, { ModSource::Bass, "macroB", 0.2f },
            { ModSource::Transient, "fx_chromatic_amt", 0.6f, 0, 200 }, { ModSource::High, "fx_glow_amt", 0.3f } } },

        { "02 Organic Flux - Deep Breath",
          { { scene, 1 }, { palette, 2 }, { macroA, 0.45f }, { macroD, 0.35f },
            { on("vignette"), 1 }, { amt("vignette"), 0.5f }, { on("glow"), 1 }, { amt("glow"), 0.25f } },
          { { ModSource::Bass, "macroA", 0.3f, 10, 300 }, { ModSource::Mid, "macroC", 0.2f },
            { ModSource::Centroid, "colorShift", 0.2f, 50, 500 } } },

        { "03 Infinite Tunnel - Hyperdrive",
          { { scene, 2 }, { palette, 4 }, { macroA, 0.3f }, { macroB, 0.6f }, { macroC, 0.65f }, { macroD, 0.6f },
            { on("trails"), 1 }, { amt("trails"), 0.55f }, { on("rgbsplit"), 1 }, { amt("rgbsplit"), 0.1f } },
          { { ModSource::Energy, "macroC", 0.25f, 50, 400 }, { ModSource::Transient, "fx_rgbsplit_amt", 0.5f, 0, 180 },
            { ModSource::Kick, "macroA", 0.2f, 0, 150 } } },

        { "04 Fractal Temple - Sacred Geometry",
          { { scene, 3 }, { palette, 2 }, { macroA, 0.4f }, { macroB, 0.6f }, { macroD, 0.45f }, { syncDiv, 2 },
            { on("glow"), 1 }, { amt("glow"), 0.4f }, { on("vignette"), 1 }, { amt("vignette"), 0.4f } },
          { { ModSource::Kick, "macroD", 0.3f, 0, 200 }, { ModSource::Bass, "macroC", 0.1f },
            { ModSource::SyncLFO, "hue", 0.05f, 0, 0 } } },

        { "05 Acid Matrix - 303 Squelch",
          { { scene, 4 }, { palette, 1 }, { macroA, 0.5f }, { macroB, 0.7f }, { macroC, 0.8f }, { macroD, 0.6f },
            { on("rgbsplit"), 1 }, { amt("rgbsplit"), 0.12f }, { on("noise"), 1 }, { amt("noise"), 0.12f } },
          { { ModSource::Centroid, "macroB", 0.35f, 20, 200 }, { ModSource::Kick, "fx_rgbsplit_amt", 0.5f, 0, 120 },
            { ModSource::Beat, "brightness", 0.12f, 0, 150 } } },

        { "06 Liquid Dream - Mercury",
          { { scene, 5 }, { palette, 5 }, { macroB, 0.55f }, { macroD, 0.45f },
            { on("glow"), 1 }, { amt("glow"), 0.3f }, { on("warp"), 1 }, { amt("warp"), 0.12f } },
          { { ModSource::Bass, "macroB", 0.3f, 10, 250 }, { ModSource::High, "fx_glow_amt", 0.3f } } },

        { "07 Neural Bloom - Synapse Fire",
          { { scene, 6 }, { palette, 2 }, { macroA, 0.45f }, { macroB, 0.6f },
            { on("glow"), 1 }, { amt("glow"), 0.55f }, { p2("glow"), 0.2f }, { on("trails"), 1 }, { amt("trails"), 0.45f } },
          { { ModSource::Transient, "macroD", 0.5f, 0, 250 }, { ModSource::Bass, "macroC", 0.3f } } },

        { "08 Psychedelic Void - Event Horizon",
          { { scene, 7 }, { palette, 7 }, { macroA, 0.6f }, { macroB, 0.7f }, { macroD, 0.5f },
            { on("feedback"), 1 }, { amt("feedback"), 0.55f }, { on("vignette"), 1 }, { amt("vignette"), 0.6f } },
          { { ModSource::Kick, "macroD", 0.4f, 0, 200 }, { ModSource::Energy, "macroC", 0.3f, 50, 400 } } },

        { "09 Image Template - Logo Mandala",
          { { scene, 7 }, { palette, 2 }, { tplEnable, 1 }, { tplMode, 1 }, { tplMix, 0.9f }, { macroB, 0.4f },
            { on("glow"), 1 }, { amt("glow"), 0.35f } },
          {}, true },

        { "10 Image Template - Kaleido Tunnel",
          { { scene, 2 }, { palette, 4 }, { tplEnable, 1 }, { tplMode, 2 }, { tplBlend, 1 }, { tplMix, 0.8f },
            { "tplSymCount", 8 } },
          {}, true },

        { "11 Mono - Silver Kaleido",
          { { scene, 0 }, { palette, 0 }, { macroA, 0.35f }, { macroB, 0.75f }, { macroD, 0.3f },
            { on("glow"), 1 }, { amt("glow"), 0.3f }, { on("noise"), 1 }, { amt("noise"), 0.1f } },
          { { ModSource::Kick, "intensity", 0.25f, 0, 150 } } },

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
