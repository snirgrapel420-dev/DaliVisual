// 17 IMAGE REACTOR — your image IS the visual (Photism-style), driven by the sound itself.
//   Motion comes from the band clocks (uBassTime, uMidTime, ...) and the spectrum — never from a
//   tempo grid: no sound, no motion.
//   Modes: 0 Kaleidoscope · 1 Liquid · 2 Tunnel · 3 Spectral Slices · 4 Droste · 5 Glitch ·
//          6 Depth 3D · 7 Neon Outline
//   Image DNA: R luminance · G edges · B distance to edges · A presence (foreground)
// Macros: A Motion · B Reactivity · C Zoom · D Trails

uniform sampler2D uDNA;          // unit 2
uniform sampler2D uImgColor;     // unit 3
uniform float uImgAspect, uHasImage, uImgMotion, uImgMask;   // uImgMask: 1 = image has transparency
uniform int   uImgMode;
uniform float uTScale, uTAngle, uTSymCount, uTWarp, uTTwist, uTFeedback, uTDetail, uTDepth;
uniform float uTColorExtract, uTEdge, uTReact, uTDistortion;

// ---------------------------------------------------------------------------------------
vec2 fitSize() { return uImgAspect >= 1.0 ? vec2(1.0, 1.0 / uImgAspect) : vec2(uImgAspect, 1.0); }
vec2 mirrorUV(vec2 uv) { return 1.0 - abs(1.0 - mod(uv, 2.0)); }            // seamless mirrored tiling
vec2 toUV(vec2 q) { return q / fitSize() * vec2(1.0, -1.0) + 0.5; }

// placeholder when no image is loaded: a soft glowing disc so the scene is never empty
vec4 placeholderAt(vec2 uv)
{
    vec2 c = uv - 0.5;
    float r = length(c);
    float disc = smoothstep(0.32, 0.30, r);
    float ring = smoothstep(0.012, 0.0, abs(r - 0.36));
    vec3 col = palette(r * 1.5 + uPalShift) * (disc * (0.35 + 0.5 * (1.0 - r * 2.5)) + ring);
    return vec4(col, disc);
}

vec4 dnaAtUV(vec2 uv)
{
    if (uHasImage < 0.5) { vec4 p = placeholderAt(mirrorUV(uv)); float l = dot(p.rgb, vec3(0.33)); return vec4(l, 0.0, 0.0, p.a); }
    return texture(uDNA, mirrorUV(uv));
}

vec3 imgAtUV(vec2 uv)
{
    uv = mirrorUV(uv);
    if (uHasImage < 0.5) return placeholderAt(uv).rgb;
    vec3 c = texture(uImgColor, uv).rgb;
    vec4 d = texture(uDNA, uv);
    // palette tint: recolour by luminance (0 = the original photo colours)
    vec3 tint = palette(d.r * 0.8 + uPalShift + 0.03 * uMidTime) * (0.2 + 1.1 * d.r);
    c = mix(c, tint, uTColorExtract);
    // glowing outlines that light up with the upper mids
    c += palette(d.b * 2.0 + uPalShift + 0.5) * d.g * uTEdge * (0.4 + 1.6 * spec(0.55 + 0.35 * d.r) * uTReact);
    // background (transparent logos / low presence) sinks into a dark tinted void
    vec3 bg = palette(0.6 + uPalShift) * 0.05 * (0.4 + uBass * uTReact);
    return mix(c, mix(bg, c, clamp(d.a * 1.2, 0.0, 1.0)), uImgMask);   // photos stay whole; logos float in the void
}

vec3 img(vec2 q) { return imgAtUV(toUV(q)); }
vec4 dna(vec2 q) { return dnaAtUV(toUV(q)); }

vec2 kfold(vec2 p, float sym)
{
    if (sym < 1.5) return p;
    float a = atan(p.y, p.x), r = length(p), seg = TAU / sym;
    a = abs(mod(a, seg) - 0.5 * seg);
    return r * vec2(cos(a), sin(a));
}

// ---------------------------------------------------------------------------------------
vec3 render(vec2 p)
{
    float R = uTReact * (0.3 + 1.4 * uMacro.y);                        // audio reactivity
    float M = (0.2 + 1.6 * uImgMotion) * (0.3 + 1.4 * uMacro.x);       // motion amount (scales the band clocks)
    float zoom = 1.0 / max(uTScale * (0.5 + uMacro.z), 0.1);
    float sym = max(1.0, floor(uTSymCount + 0.5));
    vec3 col = vec3(0.0);

    if (uImgMode == 0)                                   // KALEIDOSCOPE
    {
        float r = length(p);
        vec2 q = rot(uTAngle + uMidTime * 0.08 * M + 0.35 * uSnare * R) * p;
        q = kfold(q, sym);
        q *= (1.0 - 0.12 * uKick * R) * zoom;
        q = rot(uTTwist * r * 2.5) * q;
        q.x += uBassTime * 0.06 * M;                     // the image streams through the mirrors with the bass
        col = img(q);
        col *= 0.65 + 0.9 * spec(clamp(r * 0.9, 0.0, 1.0)) * R;        // radius = frequency
    }
    else if (uImgMode == 1)                              // LIQUID
    {
        vec2 q = p * zoom;
        float rr = length(p);
        vec2 w = vec2(fbm(q * 2.0 + vec2(0.0, uMidTime * 0.25 * M), 4), fbm(q * 2.0 + vec2(5.2, 1.3) - uMidTime * 0.2 * M, 4)) - 0.5;
        q += w * (0.05 + uTWarp) * (0.35 + 1.4 * uBass * R);
        q += normalize(p + 1e-5) * 0.035 * uKick * R * sin(rr * 26.0 - uBassTime * 5.0);
        q = rot(uTAngle + uTTwist * rr + 0.25 * uSnare * R * sin(rr * 6.0)) * q;
        float ca = (0.002 + 0.03 * uTDistortion) * (0.3 + 1.5 * uHigh * R);
        col = vec3(img(q + vec2(ca, 0.0)).r, img(q).g, img(q - vec2(ca, 0.0)).b);
    }
    else if (uImgMode == 2)                              // TUNNEL
    {
        float r = length(p);
        float a = atan(p.y, p.x) + uTAngle + uTTwist * 0.4 / max(r, 0.05) + uMidTime * 0.03 * M;
        float n = max(1.0, floor(sym * 0.5 + 0.5));
        float depth = 0.3 / max(r, 1e-3) * zoom + uBassTime * 0.3 * M;
        vec2 uv = vec2(a / TAU * n, depth);
        col = imgAtUV(uv);
        float k = floor(depth * 2.0);
        float band = spec(fract(k * 0.137) * 0.9 + 0.03);
        col *= 0.4 + 1.2 * band * R * smoothstep(0.0, 0.25, 0.5 - abs(fract(depth * 2.0) - 0.5)) + 0.4;
        col += palette(k * 0.137 + uPalShift) * exp(-abs(fract(depth * 2.0) - 0.5) * 30.0) * band * R * 0.6;
        col *= smoothstep(0.0, 0.4, r) * (1.0 + 0.5 * uKick * R);
    }
    else if (uImgMode == 3)                              // SPECTRAL SLICES — each strip is a frequency
    {
        vec2 q = rot(uTAngle) * p * zoom;
        float n = floor(12.0 + uTDetail * 52.0);
        float sy = (q.y / fitSize().y) * 0.5 + 0.5;      // 0 bottom .. 1 top
        float s = floor(sy * n);
        float band = spec(0.02 + clamp(s / n, 0.0, 1.0) * 0.93);
        float dir = mod(s, 2.0) < 0.5 ? 1.0 : -1.0;
        q.x += dir * (band - 0.2) * (0.1 + uTWarp) * 0.55 * R;
        q.x += dir * uHighTime * 0.01 * M;
        float glitch = step(0.93, hash12(vec2(s, floor(uHighTime * 10.0))));
        q.x += glitch * uHat * R * 0.15 * dir;
        col = img(q) * (0.35 + 1.4 * band * R);
        col *= smoothstep(0.0, 0.06, fract(sy * n)) * smoothstep(1.0, 0.94, fract(sy * n)) * 0.3 + 0.7;
    }
    else if (uImgMode == 4)                              // DROSTE — the image nested in itself, forever
    {
        float r = length(p), a = atan(p.y, p.x);
        float s = log(1.8 + uTDepth * 2.2);
        float lz = log(max(r, 1e-4)) - uLevelTime * 0.3 * M - 0.2 * uKick * R;
        float k = floor(lz / s);
        float f = lz - k * s;
        a += uTTwist * lz * 0.6 + uTAngle + k * 0.15;
        vec2 q = exp(f) * 0.45 * vec2(cos(a), sin(a)) * zoom;
        col = img(q) * (0.4 + 1.2 * spec(fract(-k * 0.13) * 0.9 + 0.03) * R);   // each level = a frequency
    }
    else if (uImgMode == 5)                              // GLITCH
    {
        vec2 q = p * zoom;
        vec2 cells = vec2(8.0, 6.0) * (1.0 + uTDetail * 3.0);
        vec2 b = floor(vUV * cells);
        float h = hash12(b + floor(uHighTime * 5.0));
        float band = spec(h * 0.9 + 0.05);
        q.x += step(0.55, h) * (band - 0.3) * (0.1 + uTWarp) * 0.6 * R * sign(h - 0.77);
        q.y += uKick * R * 0.06 * step(0.7, hash12(vec2(b.y, floor(uBassTime * 8.0))));
        float ca = (0.003 + 0.04 * uSnare * R) * (0.3 + 2.0 * uTDistortion);
        col = vec3(img(q + vec2(ca, 0.0)).r, img(q).g, img(q - vec2(ca, 0.0)).b);
        col = mix(col, 1.0 - col, step(0.965, h) * uHat * R);
        col *= 0.88 + 0.12 * sin(gl_FragCoord.y * 1.6);
    }
    else if (uImgMode == 6)                              // DEPTH 3D — luminance extruded into relief
    {
        vec2 q = p * zoom;
        vec2 cam = vec2(sin(uMidTime * 0.3 * M), cos(uMidTime * 0.23 * M)) * 0.05 * (0.4 + 1.2 * uBass * R) + vec2(uPan * 0.04, 0.0);
        float depth = (0.3 + 1.7 * uTDepth) * (1.0 + 0.6 * uBass * R);
        vec2 qq = q;
        for (int i = 0; i < 40; i++)                     // steep parallax: walk until the surface
        {
            float layer = 1.0 - float(i) / 39.0;
            qq = q + cam * layer * depth;
            if (dna(qq).r >= layer) break;
        }
        float e = 0.004;
        float hc = dna(qq).r;
        vec3 n = normalize(vec3(dna(qq - vec2(e, 0.0)).r - dna(qq + vec2(e, 0.0)).r,
                                dna(qq - vec2(0.0, e)).r - dna(qq + vec2(0.0, e)).r, 0.08 / depth));
        vec3 l = normalize(vec3(cos(uMidTime * 0.2), sin(uMidTime * 0.2), 0.8));
        float dif = max(dot(n, l), 0.0);
        float sp = pow(max(dot(reflect(-l, n), vec3(0.0, 0.0, 1.0)), 0.0), 20.0 + 60.0 * uHigh);
        col = img(qq) * (0.35 + 0.9 * dif) + vec3(sp) * (0.2 + 0.8 * uHigh * R);
        col += palette(hc + uPalShift) * smoothstep(0.7, 1.0, hc) * uKick * R * 0.8;
    }
    else                                                 // NEON OUTLINE — contours echo outward
    {
        for (int k = 0; k < 6; k++)
        {
            float fk = float(k);
            float sc = (1.0 + fk * 0.33) * (1.0 - 0.1 * uKick * R);
            float dir = mod(fk, 2.0) < 0.5 ? 1.0 : -1.0;
            vec2 q = rot(uTAngle + fk * uTTwist * 0.35 + dir * uMidTime * 0.04 * M) * p * zoom * sc;
            q = kfold(q, sym);
            vec4 d = dna(q);
            float band = specBand(fk / 6.0, (fk + 1.0) / 6.0);
            float iso = smoothstep(0.02, 0.0, abs(d.b - fract(uLevelTime * 0.15 * M + fk * 0.1) * 0.3));
            col += palette(fk * 0.15 + uPalShift) * (d.g * (0.25 + 2.2 * band * R) + iso * band * 0.6 * R) * (1.0 - fk / 7.0);
        }
        vec2 q0 = p * zoom;
        col += img(q0) * 0.22 * dna(q0).a;
    }
    return col;
}

void main()
{
    vec2 p = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    vec3 col = render(p);
    // photos are already bright: pre-compensate the output's filmic lift so they keep depth and contrast
    col = pow(max(col, 0.0), vec3(1.35)) * 1.1;
    col *= uIntensity * 1.15;

    // trails: the previous frame lingers (slightly expanding), so motion leaves light paths
    vec2 fq = (vUV - 0.5) * (0.994 - 0.01 * uKick * uTReact) + 0.5;
    vec3 prev = texture(uPrev, fq).rgb;
    col = max(col, prev * clamp(uTFeedback + (uMacro.w - 0.5) * 0.5, 0.0, 0.97));
    fragColor = vec4(col, 1.0);
}
