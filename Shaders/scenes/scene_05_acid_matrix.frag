// 05 — ACID MATRIX
// Macros: A Grid Density · B Squelch · C Perspective · D Sharpness
float polyDist(vec2 p, float n)
{
    float a  = atan(p.y, p.x);
    float an = mod(a + PI / n, TAU / n) - PI / n;
    return length(p) * cos(an);
}

void main()
{
    vec2 p = centered();

    // 303-style squelch: resonant warp whose frequency follows the spectral centroid
    float sq = uMacro.y * (0.35 + uCentroid * 1.6 + uMid);
    p.x += sq * 0.07 * sin(p.y * (6.0 + uCentroid * 34.0) + uTime * 3.1);
    p.y += sq * 0.05 * sin(p.x * (5.0 + uCentroid * 22.0) - uTime * 2.3);

    // perspective floor/ceiling
    float persp = uMacro.z;
    float y = abs(p.y) + 0.035;
    vec2  g = mix(p * 1.5, vec2(p.x / y * 0.35, 0.35 / y), persp);
    g.y += uTime * 0.6 + beatTravel() * 0.5;

    float dens = mix(2.0, 9.0, uMacro.x);
    g *= dens * 0.5;

    vec2  id = floor(g);
    vec2  f  = fract(g) - 0.5;
    float h  = hash12(id + floor(uBeatClock / 4.0) * 17.0);
    if (h > 0.5) f.x = -f.x;

    float sharp = mix(25.0, 160.0, uMacro.w);
    float dDiag = abs(abs(f.x + f.y) - 0.5) * 0.7071;
    float dGrid = 0.5 - max(abs(f.x), abs(f.y));
    float lines = exp(-dDiag * sharp * 0.35) + 0.55 * exp(-dGrid * sharp * 0.6);

    float aa   = clamp(0.35 / (fwidth(g.y) * sharp * 0.05 + 1e-4), 0.0, 1.0);   // fade sub-pixel cells
    lines *= aa;
    float fade = mix(1.0, smoothstep(0.02, 0.4, y - 0.035), persp);
    float lit  = step(0.82, hash12(id + 3.1)) * uBeat;          // cells lit on the beat
    vec3 col = palette(0.1 + h * 0.25 + uCentroid * 0.5) * lines * fade * (0.5 + uIntensity)
             + palette(0.45) * lit * fade * 0.5;

    // central acid shapes: nested triangles pulsing with the kick
    vec2 c = centered() * rot(uTime * 0.35 + TAU / 3.0 * beatEase(uBeatPhase));
    for (int i = 0; i < 3; ++i)
    {
        float fi  = float(i);
        float rad = (0.12 + 0.09 * fi) * (1.0 + 0.45 * uKick);
        float d   = abs(polyDist(c * rot(fi * 0.6 + uTime * 0.1 * fi), 3.0) - rad);
        col += palette(0.6 + fi * 0.12) * exp(-d * mix(60.0, 220.0, uMacro.w)) * (0.5 + 2.2 * uTransient);
    }

    col += palette(0.9) * uTransient * 0.12 * lines;
    fragColor = vec4(safeHDR(col), 1.0);
}
