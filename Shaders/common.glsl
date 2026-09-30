// ============================================================================
//  DALI VISUAL — common shader header
//  Prepended (after "#version 150") to every fragment shader by ShaderLibrary.
//  All audio uniforms are already smoothed and reactivity-scaled on the CPU.
// ============================================================================

in vec2 vUV;
out vec4 fragColor;

// --- frame ---------------------------------------------------------------
uniform vec2  uRes;          // target size in pixels
uniform float uTime;         // scene time (speed/motion scaled), seconds
uniform float uAbsTime;      // wall-clock seconds since engine start (wrapped)

// --- audio features (0..1 unless stated) ---------------------------------
uniform float uBass;         // low band envelope
uniform float uMid;          // mid band envelope
uniform float uHigh;         // high band envelope
uniform float uEnergy;       // overall energy
uniform float uKick;         // kick envelope (fast attack / decay)
uniform float uTransient;    // transient burst envelope
uniform float uBeat;         // beat pulse (1 on beat, decays)
uniform float uCentroid;     // spectral centroid (log scaled)
uniform float uFlux;         // spectral flux
uniform float uWidth;        // stereo width
uniform float uPan;          // stereo balance  -1..1
uniform float uSnare;        // snare / clap envelope
uniform float uHat;          // hi-hat envelope
uniform float uBuild;        // 0..1 tension of a breakdown / build-up
uniform float uDrop;         // 1 at a drop (kick returns after a breakdown), decays
uniform float uActivity;     // 0 in silence .. 1 while music plays

// --- musical clock -------------------------------------------------------
uniform float uBeatPhase;    // 0..1 within the current beat
uniform float uBarPhase;     // 0..1 within the current bar
uniform float uSyncPhase;    // 0..1 within the selected sync division
uniform float uBeatClock;    // continuous beat count (wrapped at 256)

// --- scene controls ------------------------------------------------------
uniform vec4  uMacro;        // scene macros A..D (0..1)
uniform float uIntensity;    // global intensity

// --- colour --------------------------------------------------------------
uniform vec3  uPalA;
uniform vec3  uPalB;
uniform vec3  uPalC;
uniform vec3  uPalD;
uniform float uPalShift;
uniform float uColorAmount;

// --- textures ------------------------------------------------------------
uniform sampler2D uTex;      // effect input / primary input
uniform sampler2D uPrev;     // this pass' own previous frame (feedback)

#define PI  3.14159265359
#define TAU 6.28318530718

mat2 rot(float a) { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float vnoise(vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p, int octaves)
{
    float v = 0.0, a = 0.5;
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    for (int i = 0; i < 8; ++i)
    {
        if (i >= octaves) break;
        v += a * vnoise(p);
        p = m * p;
        a *= 0.5;
    }
    return v;
}

// Musical ease: holds, then snaps forward — used for beat-locked motion.
float beatEase(float ph) { float x = clamp(ph, 0.0, 1.0); return 1.0 - pow(1.0 - x, 4.0); }

// Continuous musical travel: whole beats + eased fraction (monotonic, seamless).
float beatTravel() { return floor(uBeatClock) + beatEase(fract(uBeatClock)); }

// Palette lookup (cosine palette), respects Color Amount (0 = monochrome).
vec3 palette(float t)
{
    vec3 c = uPalA + uPalB * cos(TAU * (uPalC * t + uPalD + uPalShift));
    c = clamp(c, 0.0, 1.0);
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    return mix(vec3(l), c, uColorAmount);
}

// Aspect-correct centred coordinates, y in -0.5..0.5
vec2 centered() { return (gl_FragCoord.xy - 0.5 * uRes) / uRes.y; }

vec2 screenUV() { return gl_FragCoord.xy / uRes; }

vec3 safeHDR(vec3 c) { return clamp(c, 0.0, 8.0); }

// smoothstep with descending edges (defined behaviour on every driver)
float smoothstepR(float e0, float e1, float x) { return 1.0 - smoothstep(e1, e0, x); }
