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

inline const std::array<SceneInfo, 23>& sceneLibrary();

/** Index of the Image Reactor scene (the scene that renders the loaded image itself). */
constexpr int kImageSceneIndex = 16;

inline const std::array<SceneInfo, 23>& sceneLibrary()
{
    static const std::array<SceneInfo, 23> s { {
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
        { "kali",     "09  KALI CATHEDRAL",    "scene_09_kali_cathedral_frag",    { "Fold Twist", "Complexity", "Flight Speed", "Glow" },
          "Flight through a raymarched fractal cathedral. Bass drives the flight, kicks ignite the walls, the spectrum colours the architecture." },
        { "mandala",  "10  SPECTRAL MANDALA",  "scene_10_spectral_mandala_frag",  { "Symmetry", "Petal Depth", "Rotation", "Glow" },
          "The sound drawn as a mandala: every petal ring is a frequency range, the waveform circles it, hits leave expanding echoes." },
        { "julia",    "11  JULIA BLOOM",       "scene_11_julia_bloom_frag",       { "Symmetry Fold", "Detail", "Morph Speed", "Glow" },
          "A living Julia fractal. The bass morphs its shape, every contour glows with its own frequency band." },
        { "hyper",    "12  HYPERSPACE",        "scene_12_hyperspace_frag",        { "Twist", "Ornament Detail", "Speed", "Rings" },
          "Tunnel of fractal ornament. Bass is the throttle, every light ring is a frequency, kicks fire shockwaves." },
        { "oil",      "13  IRIDESCENT OIL",    "scene_13_iridescent_oil_frag",    { "Flow Scale", "Film Thickness", "Turbulence", "Gloss" },
          "Glossy liquid with thin-film rainbow colours. Mids stir it, kicks ripple it, the spectrum shifts its colours." },
        { "hyperbolic","14  HYPERBOLIC DREAM", "scene_14_hyperbolic_dream_frag",  { "Tiling Type", "Edge Width", "Flow Speed", "Depth Glow" },
          "Endless hyperbolic tiling (Escher's Circle Limit). The bass slides it through hyperbolic space, tiles glow with the spectrum." },
        { "feedback", "15  INFINITE FEEDBACK", "scene_15_infinite_feedback_frag", { "Symmetry", "Zoom", "Warp", "Trail Length" },
          "Kaleidoscopic video feedback fed only by the sound: spectrum ring, waveform and kick flashes stream into endless trails." },
        { "wavegeo",  "16  WAVEFORM GEOMETRY", "scene_16_waveform_geometry_frag", { "Layers", "Wave Amplitude", "Rotation", "Afterglow" },
          "Sacred geometry drawn by the live waveform: every polygon edge is the sound, each shape a frequency range." },
        { "image",    "17  IMAGE REACTOR",     "scene_17_image_reactor_frag",     { "Motion", "Reactivity", "Zoom", "Trails" },
          "Your own image becomes the visual: its structure, colours and contours drive generative modes (Flow Lines, Flow Paint, Pulse, ...), moved by the sound itself." },
        { "bloom",    "18  SACRED BLOOM",      "scene_18_sacred_bloom_frag",      { "Layers", "Vein Detail", "Curl", "Palette" },
          "A living mandala organism of translucent petals. Kick: it contracts like a heartbeat. Build: petals curl inward. Chaos: they fray into spores." },
        { "tidal",    "19  TIDAL CATHEDRAL",   "scene_19_tidal_cathedral_frag",   { "Architecture", "Caustics", "Drift Speed", "Palette" },
          "Drifting through an alien cathedral under the sea. Kick: a wave of light runs down the nave. Build: the camera rises and the light dims." },
        { "glass",    "20  LIQUID GLASS",      "scene_20_liquid_glass_frag",      { "Fold Scale", "Gloss", "Flow", "Palette" },
          "Molten glass folding in the dark, drawn by the lights it mirrors. Kick: an impact ripple. Chaos: the glass cracks." },
        { "mycelium", "21  MYCELIUM",          "scene_21_mycelium_frag",          { "Strands", "Branching", "Growth", "Palette" },
          "A luminous network of branching hyphae. Kick: a signal runs along the filaments. Build: it branches finer and turns amber." },
        { "hyperdim", "22  HYPERDIMENSION",    "scene_22_hyperdimension_frag",    { "Depth", "Line Weight", "Speed", "Palette" },
          "A tunnel of tesseracts turning through the fourth dimension. Kick: the tunnel expands. Snare: the geometry fractures." },
        { "solar",    "23  SOLAR TEMPLE",      "scene_23_solar_temple_frag",      { "Rings", "Engraving", "Rotation", "Palette" },
          "Engraved sacred-geometry rings before a vast sun. Kick: a pulse runs ring to ring. Build: an eclipse. Peak: a solar flare." },
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
