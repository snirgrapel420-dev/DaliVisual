// 04 — FRACTAL TEMPLE
// Macros: A Symmetry · B Fractal Depth · C Evolution · D Glow
void main()
{
    vec2 p = centered();
    float expand = 1.0 + 0.28 * uKick * uIntensity + 0.12 * uBass;
    p /= expand;

    float r = length(p);
    float a = atan(p.y, p.x);

    float seg = floor(mix(3.0, 12.0, uMacro.x) + 0.5) * 2.0;
    float sa  = TAU / seg;
    a = mod(a + uTime * 0.02, sa);
    a = abs(a - 0.5 * sa);
    vec2 q = r * vec2(cos(a), sin(a));

    float t = uTime * 0.05 * (0.25 + uMacro.z * 1.5);
    vec2  c = vec2(-0.56 + 0.07 * sin(t), -0.63 + 0.07 * cos(t * 1.31))
            + 0.04 * vec2(uMid, uBass);

    vec2  z = q * (1.25 + 0.1 * uBass);
    float trap = 1e3, trap2 = 1e3, sum = 0.0;
    int it = 6 + int(uMacro.y * 8.0);
    for (int i = 0; i < 14; ++i)
    {
        if (i >= it) break;
        z = abs(z) / max(dot(z, z), 1e-4) + c;
        trap  = min(trap,  abs(z.x * z.y));
        trap2 = min(trap2, abs(length(z) - 1.0));
        sum  += exp(-dot(z, z) * 0.5);
    }

    float v1 = exp(-trap * 22.0);
    float v2 = exp(-trap2 * 30.0);
    float glow = mix(0.5, 2.2, uMacro.w);

    vec3 col = palette(sum * 0.07 + r * 0.45 + t * 0.2 + uCentroid * 0.2) * v1 * glow * 0.8
             + palette(r * 0.6 + 0.5) * v2 * 0.55 * (0.35 + 1.2 * uHigh);

    // mandala rings breathing on the bar
    float rings = exp(-(0.5 - abs(fract(r * 5.0 - uBarPhase) - 0.5)) * 30.0);
    col += palette(0.7 + r) * rings * 0.25 * (0.3 + uMid);

    // beat-reactive expansion wave
    col += palette(0.2) * uBeat * 0.6 * exp(-abs(r - beatEase(uBeatPhase) * 0.8) * 40.0);

    col *= smoothstepR(1.3, 0.15, r);
    col *= 0.5 + 0.8 * uIntensity;
    fragColor = vec4(safeHDR(col), 1.0);
}
