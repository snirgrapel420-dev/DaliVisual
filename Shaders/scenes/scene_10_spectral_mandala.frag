// 10 SPECTRAL MANDALA — the sound drawn as sacred geometry.
//   Each petal ring is a frequency range (inner = bass ... outer = highs) and blooms with its level.
//   The waveform itself runs around the mandala as a glowing ring.
//   kick: centre flare + rings kick outward · snare: counter-rotation jolt · hi-hat: spokes flicker
// Macros: A Symmetry · B Petal Depth · C Rotation · D Glow

vec3 layer(vec2 p)
{
    float r = length(p);
    float sym = floor(6.0 + uMacro.x * 18.0);
    float seg = TAU / sym;
    float spin = uMidTime * (0.05 + 0.25 * uMacro.z);
    vec3 col = vec3(0.0);

    // --- spectral petal rings --------------------------------------------------------
    for (int k = 0; k < 7; k++)
    {
        float fk = float(k);
        float lvl = specBand(fk / 7.0, (fk + 1.0) / 7.0);
        float dir = mod(fk, 2.0) < 0.5 ? 1.0 : -1.0;
        float a = atan(p.y, p.x) + dir * (spin * (1.0 + 0.3 * fk) + 0.25 * uSnare);
        float af = abs(mod(a, seg) - 0.5 * seg) / seg;                   // 0 at petal centre .. 0.5 at edge
        float R = 0.09 + 0.085 * fk + 0.03 * uKick * (1.0 - fk / 7.0);
        float petal = R + (0.012 + (0.05 + 0.08 * uMacro.y) * lvl) * cos(af * PI * (1.0 + mod(fk, 3.0)));
        float d = abs(r - petal);
        float w = 0.0015 + 0.004 * lvl;
        float line = smoothstep(w * 2.0, 0.0, d);
        float glow = exp(-d * (60.0 - 30.0 * uMacro.w)) * (0.15 + 0.85 * lvl);
        vec3 c = palette(fk * 0.13 + uPalShift + 0.02 * uMidTime);
        col += c * (line * (0.35 + 1.8 * lvl) + glow * 0.9);
        // petal fill: soft glass between this ring and its petal edge
        col += c * smoothstep(petal, petal - 0.04, r) * smoothstep(petal - 0.09, petal - 0.04, r) * lvl * 0.25;
    }

    // --- waveform ring ------------------------------------------------------------------
    {
        float a = atan(p.y, p.x) - spin * 0.5;
        float u = fract(a / TAU * 2.0);                                   // wave wraps twice around
        float wr = 0.72 + 0.07 * wave(u) * (0.4 + 0.6 * uEnergy);
        float d = abs(r - wr);
        col += palette(0.55 + uPalShift) * (smoothstep(0.004, 0.0, d) * 1.4 + exp(-d * 45.0) * 0.35) * uActivity;
    }

    // --- spokes on the segment borders, flickering with the hats --------------------------
    {
        float a = atan(p.y, p.x) + spin;
        float af = abs(mod(a + 0.5 * seg, seg) - 0.5 * seg);
        float sp = smoothstep(0.004, 0.0, af * r) * smoothstep(0.04, 0.12, r) * smoothstep(0.8, 0.6, r);
        col += palette(0.85 + uPalShift) * sp * (0.12 + 1.2 * uHat);
    }

    // --- centre flare ----------------------------------------------------------------------
    col += palette(0.05 + uPalShift) * exp(-r * (14.0 - 8.0 * uKick)) * (0.3 + 1.6 * uKick);
    // background halo tinted by the bass
    col += palette(0.3 + uPalShift) * exp(-r * 2.5) * 0.06 * (0.5 + uBass);
    return col;
}

void main()
{
    vec3 col = vec3(0.0);
    // 2x2 rotated-grid supersampling: crisp hairlines at any resolution
    for (int i = 0; i < 4; i++)
    {
        vec2 o = vec2(float(i & 1), float(i >> 1)) - 0.5;
        o = vec2(o.x * 0.9 - o.y * 0.4, o.x * 0.4 + o.y * 0.9) * 0.5;
        vec2 p = (gl_FragCoord.xy + o - 0.5 * uRes) / uRes.y;
        p *= 1.0 - 0.04 * uKick;
        col += layer(p);
    }
    col *= 0.25 * uIntensity * 1.3;

    // feedback echo: last frame expands outward and turns, so every hit leaves rings flying into space
    vec2 q = vUV - 0.5;
    q.x *= uRes.x / uRes.y;
    q *= 0.982 - 0.02 * uKick;
    q *= rot(0.004 + 0.01 * uSnare);
    q.x /= uRes.x / uRes.y;
    vec3 echo = texture(uPrev, q + 0.5).rgb;
    echo = mix(echo, echo.gbr, 0.08);                                   // colour drifts in the echoes
    col += max(echo - 0.006, 0.0) * (0.60 + 0.16 * uMacro.w) * mix(0.9, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
