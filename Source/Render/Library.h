#pragma once
// ============================================================================
//  Library — static descriptions of the 8 scenes and 20 effects.
//  Pure data (no JUCE / GL) so parameters, UI, engine and tools share it.
// ============================================================================
#include <array>

namespace dali
{
struct SceneInfo
{
    const char* id;
    const char* name;
    const char* resource;          // BinaryData resource name
    const char* macro[4];          // scene-specific names of macros A..D
    const char* description;
};

inline const std::array<SceneInfo, 8>& sceneLibrary()
{
    static const std::array<SceneInfo, 8> s { {
        { "kinetic",  "01  KINETIC KALEIDO",   "scene_01_kinetic_kaleido_frag",  { "Segments", "Fold Complexity", "Rotation", "Mirror Feedback" },
          "Kaleidoscopic folding geometry. Kick scale pulses, bass deforms, beat-locked segment rotation." },
        { "organic",  "02  ORGANIC FLUX",      "scene_02_organic_flux_frag",     { "Warp Depth", "Detail", "Flow", "Folds" },
          "Domain-warped fluid organism. Bass expands the warp, highs add micro detail." },
        { "tunnel",   "03  INFINITE TUNNEL",   "scene_03_infinite_tunnel_frag",  { "Shape", "Wall Pattern", "Travel Speed", "Trails" },
          "Endless tunnel of glowing panels, travel synced to the beat, walls deformed by frequency." },
        { "temple",   "04  FRACTAL TEMPLE",    "scene_04_fractal_temple_frag",   { "Symmetry", "Fractal Depth", "Evolution", "Glow" },
          "Sacred-geometry mandala over an evolving kaliset fractal, expanding with every kick." },
        { "acid",     "05  ACID MATRIX",       "scene_05_acid_matrix_frag",      { "Grid Density", "Squelch", "Perspective", "Sharpness" },
          "Sharp 303-inspired grids and triangles, squelch warp tracks the spectral centroid." },
        { "liquid",   "06  LIQUID DREAM",      "scene_06_liquid_dream_frag",     { "Wave Scale", "Refraction", "Viscosity", "Dream Feedback" },
          "Refractive liquid surface with caustics and dreamy smear feedback." },
        { "neural",   "07  NEURAL BLOOM",      "scene_07_neural_bloom_frag",     { "Density", "Connections", "Growth", "Particle Glow" },
          "Growing neural network: nodes, synaptic pulses, transient bursts and sparks." },
        { "void",     "08  PSYCHEDELIC VOID",  "scene_08_psychedelic_void_frag", { "Particle Density", "Nebula", "Depth Speed", "Chromatic" },
          "Dark infinite depth, particle fields and fractal nebula with chromatic distortion." },
    } };
    return s;
}

struct EffectInfo
{
    const char* id;
    const char* name;
    const char* resource;
    const char* p1Name;
    const char* p2Name;
    bool  stateful;                 // needs its own history buffer
    float defAmt, defP2;
};

inline const std::array<EffectInfo, 20>& effectLibrary()
{
    static const std::array<EffectInfo, 20> e { {
        { "blur",        "Blur",                 "fx_blur_frag",          "Radius",   "Radial",     false, 0.30f, 0.0f },
        { "glow",        "Glow",                 "fx_glow_frag",          "Amount",   "Threshold",  false, 0.45f, 0.35f },
        { "feedback",    "Feedback",             "fx_feedback_frag",      "Persist",  "Zoom/Rot",   true,  0.70f, 0.60f },
        { "kaleido",     "Kaleidoscope",         "fx_kaleidoscope_frag",  "Mix",      "Segments",   false, 1.00f, 0.30f },
        { "mirror",      "Mirror",               "fx_mirror_frag",        "Mix",      "Mode",       false, 1.00f, 0.00f },
        { "twist",       "Twist",                "fx_twist_frag",         "Amount",   "Radius",     false, 0.30f, 0.50f },
        { "warp",        "Warp",                 "fx_warp_frag",          "Amount",   "Scale",      false, 0.30f, 0.40f },
        { "noise",       "Noise",                "fx_noise_frag",         "Grain",    "Size",       false, 0.25f, 0.20f },
        { "chromatic",   "Chromatic Aberration", "fx_chromatic_frag",     "Amount",   "Lateral",    false, 0.30f, 0.00f },
        { "rgbsplit",    "RGB Split",            "fx_rgbsplit_frag",      "Offset",   "Angle",      false, 0.25f, 0.00f },
        { "displace",    "Displacement",         "fx_displacement_frag",  "Depth",    "Gradient",   false, 0.30f, 0.50f },
        { "pixelate",    "Pixelation",           "fx_pixelate_frag",      "Size",     "Dots",       false, 0.40f, 0.00f },
        { "posterize",   "Posterization",        "fx_posterize_frag",     "Amount",   "Gamma",      false, 0.50f, 0.50f },
        { "invert",      "Invert",               "fx_invert_frag",        "Mix",      "Luma Only",  false, 1.00f, 0.00f },
        { "contrast",    "Contrast",             "fx_contrast_frag",      "Contrast", "Pivot",      false, 0.60f, 0.35f },
        { "brightness",  "Brightness",           "fx_brightness_frag",    "Gain",     "Lift",       false, 0.60f, 0.00f },
        { "saturation",  "Saturation",           "fx_saturation_frag",    "Amount",   "Vibrance",   false, 0.65f, 0.30f },
        { "hueshift",    "Hue Shift",            "fx_hueshift_frag",      "Offset",   "Rotate",     false, 0.10f, 0.00f },
        { "vignette",    "Vignette",             "fx_vignette_frag",      "Amount",   "Softness",   false, 0.50f, 0.50f },
        { "trails",      "Trails",               "fx_trails_frag",        "Decay",    "Colour Drift", true, 0.75f, 0.20f },
    } };
    return e;
}

inline const char* const* templateModeNames()
{
    static const char* n[] = { "Kaleidoscope", "Mandala", "Tunnel", "Recursive", "Rotating Geometry", "Organic", "Feedback Echo" };
    return n;
}
constexpr int kNumTemplateModes = 7;

inline const char* const* templateBlendNames()
{
    static const char* n[] = { "Screen", "Add", "Mask", "Replace" };
    return n;
}
constexpr int kNumTemplateBlends = 4;
} // namespace dali
