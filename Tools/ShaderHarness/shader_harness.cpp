// ============================================================================
//  DALI VISUAL — Shader Harness (Linux, headless, Mesa EGL surfaceless)
//
//  Development / CI tool. Compiles every shader exactly the way the engine
//  assembles them (#version 150 + common.glsl + source), links them against
//  the fullscreen vertex shader and renders each scene / effect / the image
//  template for N frames with a synthetic 140 BPM audio signal. Writes PPM
//  frames plus a statistics report (catches NaN, black or blown-out output).
//
//  Build:  g++ -O2 -std=c++17 shader_harness.cpp ../../Source/Image/ImageDNA.cpp -ldl -o shader_harness
//  Run:    ./shader_harness <ShadersDir> <outDir> [imageRGBA.raw w h]
// ============================================================================
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "../../Source/Image/ImageDNA.h"
#include "../../Source/Render/Palettes.h"

// ---------------------------------------------------------------- EGL / GL
typedef void* EGLDisplay; typedef void* EGLContext; typedef void* EGLConfig; typedef void* EGLSurface;
typedef int EGLint; typedef unsigned EGLBoolean; typedef unsigned EGLenum;
typedef unsigned GLenum; typedef unsigned GLuint; typedef int GLint; typedef int GLsizei;
typedef float GLfloat; typedef char GLchar; typedef unsigned char GLubyte; typedef unsigned GLbitfield;

#define FN(ret, name, args) typedef ret (*PFN_##name) args; static PFN_##name name;
FN(void*, eglGetProcAddress, (const char*))
FN(EGLDisplay, eglGetPlatformDisplayEXT, (EGLenum, void*, const EGLint*))
FN(EGLBoolean, eglInitialize, (EGLDisplay, EGLint*, EGLint*))
FN(EGLBoolean, eglBindAPI, (EGLenum))
FN(EGLContext, eglCreateContext, (EGLDisplay, EGLConfig, EGLContext, const EGLint*))
FN(EGLBoolean, eglMakeCurrent, (EGLDisplay, EGLSurface, EGLSurface, EGLContext))
FN(EGLint, eglGetError, ())
FN(GLuint, glCreateShader, (GLenum))
FN(void, glShaderSource, (GLuint, GLsizei, const GLchar* const*, const GLint*))
FN(void, glCompileShader, (GLuint))
FN(void, glGetShaderiv, (GLuint, GLenum, GLint*))
FN(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*))
FN(GLuint, glCreateProgram, ())
FN(void, glAttachShader, (GLuint, GLuint))
FN(void, glLinkProgram, (GLuint))
FN(void, glGetProgramiv, (GLuint, GLenum, GLint*))
FN(void, glGetProgramInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*))
FN(void, glUseProgram, (GLuint))
FN(GLint, glGetUniformLocation, (GLuint, const GLchar*))
FN(void, glUniform1f, (GLint, GLfloat))
FN(void, glUniform2f, (GLint, GLfloat, GLfloat))
FN(void, glUniform3f, (GLint, GLfloat, GLfloat, GLfloat))
FN(void, glUniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat))
FN(void, glUniform1i, (GLint, GLint))
FN(void, glGenVertexArrays, (GLsizei, GLuint*))
FN(void, glBindVertexArray, (GLuint))
FN(void, glGenFramebuffers, (GLsizei, GLuint*))
FN(void, glBindFramebuffer, (GLenum, GLuint))
FN(void, glFramebufferTexture2D, (GLenum, GLenum, GLenum, GLuint, GLint))
FN(GLenum, glCheckFramebufferStatus, (GLenum))
FN(void, glGenTextures, (GLsizei, GLuint*))
FN(void, glBindTexture, (GLenum, GLuint))
FN(void, glTexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*))
FN(void, glTexParameteri, (GLenum, GLenum, GLint))
FN(void, glActiveTexture, (GLenum))
FN(void, glViewport, (GLint, GLint, GLsizei, GLsizei))
FN(void, glDrawArrays, (GLenum, GLint, GLsizei))
FN(void, glReadPixels, (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*))
FN(void, glFinish, ())
FN(GLenum, glGetError, ())
FN(const GLubyte*, glGetString, (GLenum))
FN(void, glClearColor, (GLfloat, GLfloat, GLfloat, GLfloat))
FN(void, glClear, (GLbitfield))

static void* lib = nullptr;
template <typename T> static void load(T& fp, const char* name)
{
    fp = (T) dlsym(lib, name);
    if (!fp && eglGetProcAddress) fp = (T) eglGetProcAddress(name);
    if (!fp) { std::fprintf(stderr, "missing symbol %s\n", name); std::exit(2); }
}

enum : unsigned {
    GL_FRAGMENT_SHADER = 0x8B30, GL_VERTEX_SHADER = 0x8B31, GL_COMPILE_STATUS = 0x8B81, GL_LINK_STATUS = 0x8B82,
    GL_FRAMEBUFFER = 0x8D40, GL_COLOR_ATTACHMENT0 = 0x8CE0, GL_FRAMEBUFFER_COMPLETE = 0x8CD5, GL_TEXTURE_2D = 0x0DE1,
    GL_RGBA16F = 0x881A, GL_RGBA = 0x1908, GL_FLOAT = 0x1406, GL_UNSIGNED_BYTE = 0x1401, GL_TEXTURE_MIN_FILTER = 0x2801,
    GL_TEXTURE_MAG_FILTER = 0x2800, GL_LINEAR = 0x2601, GL_TEXTURE_WRAP_S = 0x2802, GL_TEXTURE_WRAP_T = 0x2803,
    GL_CLAMP_TO_EDGE = 0x812F, GL_TRIANGLES = 0x0004, GL_TEXTURE0 = 0x84C0, GL_RGBA8 = 0x8058, GL_VERSION = 0x1F02,
    GL_RENDERER = 0x1F01, GL_COLOR_BUFFER_BIT = 0x4000, GL_SHADING_LANGUAGE_VERSION = 0x8B8C
};

// ---------------------------------------------------------------- helpers
static std::string readFile(const std::string& p)
{
    std::ifstream f(p); if (!f) { std::fprintf(stderr, "cannot read %s\n", p.c_str()); std::exit(3); }
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

static int failures = 0;

static GLuint compile(GLenum type, const std::string& src, const std::string& name)
{
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    char log[8192] = {}; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
    if (!ok) { std::printf("  [FAIL] compile %s\n%s\n", name.c_str(), log); ++failures; return 0; }
    if (std::strlen(log) > 2) std::printf("  [warn] %s: %s\n", name.c_str(), log);
    return s;
}

static GLuint vertShader = 0;
static std::string commonSrc;

static GLuint program(const std::string& fragSrc, const std::string& name)
{
    std::string full = "#version 150\n" + commonSrc + "\n#line 1\n" + fragSrc;
    GLuint fs = compile(GL_FRAGMENT_SHADER, full, name);
    if (!fs) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, vertShader); glAttachShader(p, fs); glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log); std::printf("  [FAIL] link %s\n%s\n", name.c_str(), log); ++failures; return 0; }
    std::printf("  [ok]   %s\n", name.c_str());
    return p;
}

struct Target { GLuint tex = 0, fbo = 0; int w = 0, h = 0; };
static Target makeTarget(int w, int h, bool hdr)
{
    Target t; t.w = w; t.h = h;
    glGenTextures(1, &t.tex); glBindTexture(GL_TEXTURE_2D, t.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, hdr ? GL_RGBA16F : GL_RGBA8, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &t.fbo); glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::printf("FBO incomplete\n"); std::exit(4); }
    glClearColor(0, 0, 0, 0); glClear(GL_COLOR_BUFFER_BIT);
    return t;
}

struct Audio { float bass, mid, high, energy, kick, transient, beat, centroid, flux, width, pan, beatPhase, barPhase, syncPhase, beatClock; };
static Audio synthAudio(float t)
{
    Audio a{};
    float beats = t * 140.0f / 60.0f;
    a.beatClock = std::fmod(beats, 256.0f);
    a.beatPhase = beats - std::floor(beats);
    a.barPhase = (beats / 4.0f) - std::floor(beats / 4.0f);
    a.syncPhase = a.beatPhase;
    a.kick = std::exp(-a.beatPhase * 7.0f);
    a.beat = std::exp(-a.beatPhase * 5.0f);
    a.bass = 0.35f + 0.45f * a.kick;
    a.mid = 0.45f + 0.2f * std::sin(t * 1.3f);
    a.high = 0.35f + 0.25f * std::sin(t * 7.1f) * std::sin(t * 2.3f);
    a.energy = 0.55f + 0.25f * a.kick;
    a.transient = (int(beats) % 4 == 3) ? std::exp(-a.beatPhase * 10.0f) : 0.0f;
    a.centroid = 0.45f + 0.2f * std::sin(t * 0.7f);
    a.flux = 0.3f + 0.3f * a.kick;
    a.width = 0.4f; a.pan = 0.1f * std::sin(t);
    return a;
}

static void setCommon(GLuint p, int w, int h, float t, const Audio& a, const float macro[4], int palette)
{
    glUseProgram(p);
    auto U = [&](const char* n) { return glGetUniformLocation(p, n); };
    glUniform2f(U("uRes"), (float) w, (float) h);
    glUniform1f(U("uTime"), t); glUniform1f(U("uAbsTime"), t);
    glUniform1f(U("uBass"), a.bass); glUniform1f(U("uMid"), a.mid); glUniform1f(U("uHigh"), a.high);
    glUniform1f(U("uEnergy"), a.energy); glUniform1f(U("uKick"), a.kick); glUniform1f(U("uTransient"), a.transient);
    glUniform1f(U("uBeat"), a.beat); glUniform1f(U("uCentroid"), a.centroid); glUniform1f(U("uFlux"), a.flux);
    glUniform1f(U("uWidth"), a.width); glUniform1f(U("uPan"), a.pan);
    glUniform1f(U("uBeatPhase"), a.beatPhase); glUniform1f(U("uBarPhase"), a.barPhase);
    glUniform1f(U("uSyncPhase"), a.syncPhase); glUniform1f(U("uBeatClock"), a.beatClock);
    glUniform4f(U("uMacro"), macro[0], macro[1], macro[2], macro[3]);
    glUniform1f(U("uIntensity"), 0.75f);
    const dali::CosinePalette& P = dali::builtInPalette(palette);
    glUniform3f(U("uPalA"), P.a[0], P.a[1], P.a[2]); glUniform3f(U("uPalB"), P.b[0], P.b[1], P.b[2]);
    glUniform3f(U("uPalC"), P.c[0], P.c[1], P.c[2]); glUniform3f(U("uPalD"), P.d[0], P.d[1], P.d[2]);
    glUniform1f(U("uPalShift"), 0.0f); glUniform1f(U("uColorAmount"), 1.0f);
    glUniform1i(U("uTex"), 0); glUniform1i(U("uPrev"), 1);
}

static void bindTex(int unit, GLuint tex) { glActiveTexture(GL_TEXTURE0 + unit); glBindTexture(GL_TEXTURE_2D, tex); }
static void draw(const Target& t) { glBindFramebuffer(GL_FRAMEBUFFER, t.fbo); glViewport(0, 0, t.w, t.h); glDrawArrays(GL_TRIANGLES, 0, 3); }

static void savePPM(const Target& t, const std::string& path, double& meanLum, double& clipFrac, int& nanCount)
{
    std::vector<float> px(size_t(t.w) * t.h * 4);
    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glReadPixels(0, 0, t.w, t.h, GL_RGBA, GL_FLOAT, px.data());
    FILE* f = std::fopen(path.c_str(), "wb");
    std::fprintf(f, "P6\n%d %d\n255\n", t.w, t.h);
    double sum = 0; long clip = 0; nanCount = 0;
    for (int y = t.h - 1; y >= 0; --y)
        for (int x = 0; x < t.w; ++x)
        {
            const float* p = &px[(size_t(y) * t.w + x) * 4];
            unsigned char rgb[3];
            for (int c = 0; c < 3; ++c)
            {
                float v = p[c];
                if (!std::isfinite(v)) { ++nanCount; v = 0; }
                v = std::fmin(std::fmax(v, 0.0f), 1.0f);
                rgb[c] = (unsigned char) std::lround(v * 255.0f);
            }
            float l = 0.299f * rgb[0] + 0.587f * rgb[1] + 0.114f * rgb[2];
            sum += l; if (l > 250) ++clip;
            std::fwrite(rgb, 1, 3, f);
        }
    std::fclose(f);
    meanLum = sum / (double(t.w) * t.h);
    clipFrac = double(clip) / (double(t.w) * t.h);
}

static GLuint uploadRGBA32F(int w, int h, const float* data)
{
    GLuint tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::printf("usage: shader_harness <ShadersDir> <outDir> [image.rgba w h]\n"); return 1; }
    std::string dir = argv[1], out = argv[2];

    lib = dlopen("libEGL.so.1", RTLD_NOW | RTLD_GLOBAL);
    void* gl = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL); (void) gl;
    if (!lib) { std::printf("no libEGL\n"); return 2; }
    eglGetProcAddress = (PFN_eglGetProcAddress) dlsym(lib, "eglGetProcAddress");
    load(eglGetPlatformDisplayEXT, "eglGetPlatformDisplayEXT"); load(eglInitialize, "eglInitialize");
    load(eglBindAPI, "eglBindAPI"); load(eglCreateContext, "eglCreateContext"); load(eglMakeCurrent, "eglMakeCurrent");
    load(eglGetError, "eglGetError");

    EGLDisplay dpy = eglGetPlatformDisplayEXT(0x31DD, nullptr, nullptr);
    EGLint maj, min;
    if (!eglInitialize(dpy, &maj, &min)) { std::printf("eglInitialize failed %x\n", eglGetError()); return 2; }
    eglBindAPI(0x30A2);
    const EGLint attrs[] = { 0x3098, 3, 0x30FB, 2, 0x30FD, 1, 0x3038 };
    EGLContext ctx = eglCreateContext(dpy, nullptr, nullptr, attrs);
    if (!ctx || !eglMakeCurrent(dpy, nullptr, nullptr, ctx)) { std::printf("context failed %x\n", eglGetError()); return 2; }

#define L(n) load(n, #n)
    L(glCreateShader); L(glShaderSource); L(glCompileShader); L(glGetShaderiv); L(glGetShaderInfoLog);
    L(glCreateProgram); L(glAttachShader); L(glLinkProgram); L(glGetProgramiv); L(glGetProgramInfoLog);
    L(glUseProgram); L(glGetUniformLocation); L(glUniform1f); L(glUniform2f); L(glUniform3f); L(glUniform4f);
    L(glUniform1i); L(glGenVertexArrays); L(glBindVertexArray); L(glGenFramebuffers); L(glBindFramebuffer);
    L(glFramebufferTexture2D); L(glCheckFramebufferStatus); L(glGenTextures); L(glBindTexture); L(glTexImage2D);
    L(glTexParameteri); L(glActiveTexture); L(glViewport); L(glDrawArrays); L(glReadPixels); L(glFinish);
    L(glGetError); L(glGetString); L(glClearColor); L(glClear);

    std::printf("GL: %s | %s | GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER), glGetString(GL_SHADING_LANGUAGE_VERSION));

    GLuint vao; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    commonSrc = readFile(dir + "/common.glsl");
    vertShader = compile(GL_VERTEX_SHADER, readFile(dir + "/fullscreen.vert"), "fullscreen.vert");

    const char* scenes[] = { "scene_01_kinetic_kaleido", "scene_02_organic_flux", "scene_03_infinite_tunnel",
        "scene_04_fractal_temple", "scene_05_acid_matrix", "scene_06_liquid_dream", "scene_07_neural_bloom",
        "scene_08_psychedelic_void" };
    const char* fx[] = { "fx_blur", "fx_glow", "fx_feedback", "fx_kaleidoscope", "fx_mirror", "fx_twist", "fx_warp",
        "fx_noise", "fx_chromatic", "fx_rgbsplit", "fx_displacement", "fx_pixelate", "fx_posterize", "fx_invert",
        "fx_contrast", "fx_brightness", "fx_saturation", "fx_hueshift", "fx_vignette", "fx_trails" };

    std::printf("\n== compile ==\n");
    std::map<std::string, GLuint> progs;
    for (auto s : scenes) progs[s] = program(readFile(dir + "/scenes/" + s + ".frag"), s);
    for (auto s : fx) progs[s] = program(readFile(dir + "/fx/" + s + ".frag"), s);
    progs["template_layer"] = program(readFile(dir + "/template_layer.frag"), "template_layer");
    progs["template_composite"] = program(readFile(dir + "/template_composite.frag"), "template_composite");
    progs["output"] = program(readFile(dir + "/output.frag"), "output");
    progs["crossfade"] = program(readFile(dir + "/crossfade.frag"), "crossfade");
    if (failures) { std::printf("\n%d shader failures\n", failures); return 1; }

    const int W = 480, H = 270, FRAMES = 48;
    Target a = makeTarget(W, H, true), b = makeTarget(W, H, true), o = makeTarget(W, H, true);
    Target fxA = makeTarget(W, H, true), fxB = makeTarget(W, H, true);

    auto runOutput = [&](GLuint src, float t, const Audio& au) {
        float m[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
        setCommon(progs["output"], W, H, t, au, m, 2);
        GLuint p = progs["output"];
        glUniform1f(glGetUniformLocation(p, "uHue"), 0.0f); glUniform1f(glGetUniformLocation(p, "uSaturation"), 1.0f);
        glUniform1f(glGetUniformLocation(p, "uBrightness"), 1.0f); glUniform1f(glGetUniformLocation(p, "uContrast"), 1.0f);
        bindTex(0, src); draw(o);
    };

    std::printf("\n== render scenes ==\n");
    int sceneIdx = 0;
    for (auto s : scenes)
    {
        Target* cur = &a; Target* prev = &b;
        glBindFramebuffer(GL_FRAMEBUFFER, a.fbo); glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, b.fbo); glClear(GL_COLOR_BUFFER_BIT);
        float macro[4] = { 0.5f, 0.55f, 0.6f, 0.55f };
        float t = 0; Audio au{};
        for (int f = 0; f < FRAMES; ++f)
        {
            t = 3.0f + f / 60.0f * 2.0f;
            au = synthAudio(t);
            static const int scenePal[8] = { 2, 7, 2, 2, 1, 5, 2, 2 };
            setCommon(progs[s], W, H, t, au, macro, scenePal[sceneIdx]);
            bindTex(1, prev->tex);
            draw(*cur);
            std::swap(cur, prev);
        }
        runOutput(prev->tex, t, au);
        double lum, clip; int nans;
        savePPM(o, out + "/" + s + ".ppm", lum, clip, nans);
        bool bad = nans > 0 || lum < 4.0 || clip > 0.6;
        std::printf("  %-28s meanLum %6.1f  clipped %5.1f%%  NaN %d %s\n", s, lum, clip * 100.0, nans, bad ? "<-- CHECK" : "");
        if (nans) ++failures;
        ++sceneIdx;
    }

    // effects on top of scene 01's final frame (a/b hold last scene; re-render scene 01 into 'a')
    std::printf("\n== render effects ==\n");
    {
        float macro[4] = { 0.5f, 0.55f, 0.6f, 0.0f };
        Audio au = synthAudio(4.0f);
        setCommon(progs["scene_01_kinetic_kaleido"], W, H, 4.0f, au, macro, 2);
        bindTex(1, b.tex); draw(a);
        for (auto e : fx)
        {
            GLuint p = progs[e];
            glBindFramebuffer(GL_FRAMEBUFFER, fxB.fbo); glClear(GL_COLOR_BUFFER_BIT);
            for (int f = 0; f < 6; ++f)
            {
                setCommon(p, W, H, 4.0f + f / 60.0f, au, macro, 2);
                glUniform1f(glGetUniformLocation(p, "uAmt"), 0.7f); glUniform1f(glGetUniformLocation(p, "uP2"), 0.5f);
                bindTex(0, a.tex); bindTex(1, fxB.tex); draw(fxA);
                std::swap(fxA, fxB);
            }
            runOutput(fxB.tex, 4.0f, au);
            double lum, clip; int nans;
            savePPM(o, out + "/" + std::string(e) + ".ppm", lum, clip, nans);
            std::printf("  %-18s meanLum %6.1f  clipped %5.1f%%  NaN %d\n", e, lum, clip * 100.0, nans);
            if (nans) ++failures;
        }
    }

    // image template
    if (argc >= 6)
    {
        std::printf("\n== image template ==\n");
        int iw = std::atoi(argv[4]), ih = std::atoi(argv[5]);
        std::vector<unsigned char> rgba(size_t(iw) * ih * 4);
        FILE* f = std::fopen(argv[3], "rb"); std::fread(rgba.data(), 1, rgba.size(), f); std::fclose(f);
        dali::ImageDNA dna = dali::ImageDNA::analyse(rgba.data(), iw, ih, 512);
        std::printf("  DNA %dx%d  aspect %.3f  palette[0] %.2f %.2f %.2f\n", dna.width, dna.height, dna.aspect,
                    dna.palette[0][0], dna.palette[0][1], dna.palette[0][2]);
        GLuint dnaTex = uploadRGBA32F(dna.width, dna.height, dna.dna.data());
        GLuint colTex = uploadRGBA32F(dna.width, dna.height, dna.color.data());
        const char* modes[] = { "kaleido", "mandala", "tunnel", "recursive", "rotating", "organic", "echo" };
        GLuint lp = progs["template_layer"], cp = progs["template_composite"];
        for (int mode = 0; mode < 7; ++mode)
        {
            Target* cur = &fxA; Target* prv = &fxB;
            glBindFramebuffer(GL_FRAMEBUFFER, fxA.fbo); glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, fxB.fbo); glClear(GL_COLOR_BUFFER_BIT);
            float macro[4] = { 0.5f, 0.5f, 0.5f, 0.3f };
            Audio au{}; float t = 0;
            for (int fr = 0; fr < 24; ++fr)
            {
                t = 5.0f + fr / 30.0f; au = synthAudio(t);
                setCommon(progs["scene_08_psychedelic_void"], W, H, t, au, macro, 2);
                bindTex(1, b.tex); draw(a);
                setCommon(lp, W, H, t, au, macro, 2);
                auto U = [&](const char* n) { return glGetUniformLocation(lp, n); };
                glUniform1i(U("uTMode"), mode);
                glUniform1f(U("uImgAspect"), dna.aspect);
                glUniform1f(U("uTScale"), 0.8f); glUniform1f(U("uTAngle"), t * 0.1f); glUniform1f(U("uTSym"), 1.0f);
                glUniform1f(U("uTSymCount"), 6.0f); glUniform1f(U("uTMirror"), 0.0f); glUniform1f(U("uTKaleido"), 1.0f);
                glUniform1f(U("uTWarp"), 0.2f); glUniform1f(U("uTTwist"), 0.1f); glUniform1f(U("uTNoise"), 0.0f);
                glUniform1f(U("uTDistortion"), 0.1f); glUniform1f(U("uTFeedback"), 0.5f); glUniform1f(U("uTRecursion"), 0.4f);
                glUniform1f(U("uTEdge"), 0.3f); glUniform1f(U("uTThreshold"), 0.5f); glUniform1f(U("uTLuminance"), 0.0f);
                glUniform1f(U("uTColorExtract"), 0.4f); glUniform1f(U("uTColorAmount"), 1.0f); glUniform1f(U("uTDetail"), 0.6f);
                glUniform1f(U("uTComplexity"), 0.5f); glUniform1f(U("uTDepth"), 0.5f); glUniform1f(U("uTMotion"), t * 0.2f);
                glUniform1f(U("uTReact"), 1.0f);
                glUniform1i(U("uDNA"), 2); glUniform1i(U("uImgColor"), 3);
                bindTex(1, prv->tex); bindTex(2, dnaTex); bindTex(3, colTex);
                draw(*cur); std::swap(cur, prv);
            }
            setCommon(cp, W, H, t, au, macro, 2);
            glUniform1f(glGetUniformLocation(cp, "uTMix"), 0.9f); glUniform1i(glGetUniformLocation(cp, "uTBlend"), 0);
            glUniform1i(glGetUniformLocation(cp, "uLayer"), 4);
            bindTex(0, a.tex); bindTex(4, prv->tex); draw(b);
            runOutput(b.tex, t, au);
            double lum, clip; int nans;
            savePPM(o, out + "/template_" + modes[mode] + ".ppm", lum, clip, nans);
            std::printf("  %-12s meanLum %6.1f  clipped %5.1f%%  NaN %d\n", modes[mode], lum, clip * 100.0, nans);
            if (nans) ++failures;
        }
    }

    std::printf("\nGL error state: 0x%x\n", glGetError());
    std::printf("%s\n", failures ? "HARNESS: FAILURES" : "HARNESS: ALL PASSED");
    return failures ? 1 : 0;
}
