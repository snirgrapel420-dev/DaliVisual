// 12 HYPERSPACE — flight through a tunnel of living fractal ornament.
//   bass clock  forward speed (no bass: you hover) · kick: a shockwave of light runs down the tunnel
//   spectrum    every ring of light is a frequency band · snare: the tunnel twists · hi-hat: star streaks
// Macros: A Twist · B Ornament Detail · C Speed · D Rings

float kaliPattern(vec2 q, float detail)
{
    q = abs(fract(q) - 0.5);
    float acc = 0.0, prev = 0.0;
    for (int i = 0; i < 9; i++)
    {
        if (float(i) > 4.0 + detail * 5.0) break;
        q = abs(q) / dot(q, q) - vec2(0.92 + 0.05 * uBass, 0.86);
        float m = length(q);
        acc += exp(-abs(m - prev) * 3.0);
        prev = m;
    }
    return acc / (4.0 + detail * 5.0);
}

vec3 tunnel(vec2 p)
{
    float r = length(p);
    float a = atan(p.y, p.x);
    float travel = uBassTime * (0.4 + 1.8 * uMacro.z) + uTime * 0.12;
    float z = 0.45 / max(r, 1e-3) + travel;
    float twist = (0.15 + 0.6 * uMacro.x) * (0.35 / max(r, 0.05)) + 0.3 * uSnare + 0.05 * uMidTime;
    vec2 tc = vec2((a + twist) / TAU * 6.0, z * 0.5);

    float orn = smoothstep(0.04, 0.42, kaliPattern(tc, uMacro.y));       // calibrated range of the pattern
    vec3 col = palette(orn * 0.8 + z * 0.06 + uPalShift) * (0.06 + 1.3 * orn * orn);

    // rings of light: ring k glows with spectrum band k
    float ringZ = z * (1.0 + 2.0 * uMacro.w);
    float k = floor(ringZ);
    float fr = fract(ringZ);
    float band = spec(fract(k * 0.137) * 0.92 + 0.02);
    float ring = smoothstep(0.1, 0.0, abs(fr - 0.5) - 0.03);
    col += palette(k * 0.137 + uPalShift + 0.4) * ring * (0.08 + 2.2 * band * band);

    // kick shockwave travelling away from the viewer
    float shock = exp(-abs(z - travel - 1.0 - 6.0 * (1.0 - uKick)) * 3.0) * uKick;
    col += palette(0.05 + uPalShift) * shock * 1.5;

    // hi-hat star streaks
    float cell = floor(a / TAU * 90.0);
    float h = fract(sin(cell * 91.7) * 43758.5);
    float streak = step(0.93, h) * smoothstep(0.0, 0.6, r) * exp(-fract(z * 0.7 + h) * 7.0);
    col += palette(h + uPalShift) * streak * (0.1 + 2.0 * uHat);

    // depth: the far end is dark, near walls are bright; energy lifts the whole tunnel
    col *= smoothstep(0.0, 0.35, r) * (0.5 + 0.8 * uEnergy);
    return col;
}

void main()
{
    vec2 p = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    p += 0.01 * uKick * vec2(sin(uAbsTime * 70.0), cos(uAbsTime * 63.0));       // kick shake
    vec3 col = vec3(0.0);
    for (int s = 0; s < 4; s++)
    {
        vec2 o = (vec2(float(s & 1), float(s >> 1)) - 0.5) * 0.5 / uRes.y;
        col += tunnel(p + o);
    }
    col *= 0.25 * uIntensity * 1.2;
    fragColor = vec4(col, 1.0);
}
