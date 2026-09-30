// 16 WAVEFORM GEOMETRY — sacred geometry drawn by the sound.
//   Nested polygons whose every edge IS the live waveform (an oscilloscope bent into geometry).
//   Each polygon belongs to a frequency range (inner = bass ... outer = highs) and swells with it.
//   kick: the geometry expands · snare: polygons counter-rotate · hi-hat: vertices sparkle
// Macros: A Layers · B Wave Amplitude · C Rotation · D Afterglow

vec3 geometry(vec2 p)
{
    vec3 col = vec3(0.0);
    float r = length(p);
    float layers = 3.0 + floor(uMacro.x * 4.0 + 0.5);
    for (int k = 0; k < 7; k++)
    {
        float fk = float(k);
        if (fk >= layers) break;
        float n = (k == 0) ? 3.0 : (k == 1) ? 6.0 : (k == 2) ? 4.0 : (k == 3) ? 8.0 : (k == 4) ? 3.0 : (k == 5) ? 12.0 : 5.0;
        float lvl = specBand(fk / layers * 0.9, (fk + 1.0) / layers * 0.9 + 0.05);
        float dir = mod(fk, 2.0) < 0.5 ? 1.0 : -1.0;
        float ang = dir * (uMidTime * (0.03 + 0.12 * uMacro.z) + 0.25 * uSnare);
        float a = atan(p.y, p.x) + ang;
        float seg = TAU / n;
        float af = mod(a, seg) - 0.5 * seg;                              // -seg/2 .. seg/2 around the edge normal
        vec2 lp = r * vec2(cos(af), sin(af));
        float apo = (0.08 + 0.075 * fk) * (1.0 + 0.12 * uKick) * (0.9 + 0.35 * lvl);
        float side = 2.0 * apo * tan(0.5 * seg);
        float u = lp.y / side + 0.5;                                     // 0..1 along the edge
        float amp = (0.01 + 0.05 * uMacro.y) * (0.3 + 1.2 * lvl) * uActivity;
        float target = apo + amp * wave(u * 0.5 + fk * 0.137) * sin(clamp(u, 0.0, 1.0) * PI);
        float d = abs(lp.x - target);
        vec3 c = palette(fk * 0.14 + uPalShift + 0.01 * uMidTime);
        col += c * (smoothstep(0.0035, 0.0, d) * (0.5 + 1.5 * lvl) + exp(-d * 90.0) * 0.25 * (0.3 + lvl));
        // vertices sparkle with the hats
        float dv = length(lp - vec2(apo, 0.5 * side));
        dv = min(dv, length(lp - vec2(apo, -0.5 * side)));
        col += c * exp(-dv * 120.0) * (0.3 + 2.0 * uHat);
    }
    return col;
}

void main()
{
    vec3 col = vec3(0.0);
    for (int s = 0; s < 4; s++)
    {
        vec2 o = (vec2(float(s & 1), float(s >> 1)) - 0.5) * 0.5;
        vec2 p = (gl_FragCoord.xy + o - 0.5 * uRes) / uRes.y;
        p *= 1.0 - 0.05 * uKick;
        // three interleaved copies build star / flower figures out of the simple polygons
        col += geometry(p);
        col += geometry(rot(TAU / 12.0) * p) * 0.55;
        col += geometry(rot(-TAU / 12.0) * p * 1.08) * 0.35;
    }
    col *= 0.25 * uIntensity * 1.3;

    // Flower of Life lattice in the background, breathing with the bass
    {
        vec2 p = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y * rot(uMidTime * 0.01);
        float R = 0.16 * (1.0 + 0.06 * uBass);
        vec2 hx = vec2(1.0, 1.7320508) * R;
        vec2 g1 = mod(p, hx) - 0.5 * hx, g2 = mod(p - 0.5 * hx, hx) - 0.5 * hx;
        float dc = min(abs(length(g1) - R), abs(length(g2) - R));
        float fade = smoothstep(0.75, 0.15, length(p));
        col += palette(0.7 + uPalShift) * smoothstep(0.003, 0.0, dc) * 0.12 * fade * (0.4 + uBass);
    }
    // afterglow: a short, slightly expanding echo of the previous frame
    vec2 q = (vUV - 0.5) * (0.992 - 0.01 * uKick) + 0.5;
    col += max(texture(uPrev, q).rgb - 0.004, 0.0) * (0.55 + 0.3 * uMacro.w);
    fragColor = vec4(col, 1.0);
}
