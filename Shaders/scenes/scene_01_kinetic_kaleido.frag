// 01 — KINETIC KALEIDO
// Macros: A Segments · B Fold Complexity · C Rotation · D Mirror Feedback
void main()
{
    vec2 p = centered();
    p.x += uPan * 0.08;

    // kick pulse + bass breathing on scale
    float scale = 1.0 - 0.22 * uKick * uIntensity - 0.10 * uBass;
    p *= scale;

    float r = length(p);
    float a = atan(p.y, p.x);

    float seg = floor(mix(3.0, 16.0, uMacro.x) + 0.5);
    float sa  = TAU / seg;

    // continuous rotation + beat-locked segment steps (seamless: one segment per beat)
    float rotSpeed = (uMacro.z - 0.5) * 0.8;
    a += uTime * rotSpeed;
    a += sa * beatEase(uBeatPhase) * step(0.55, uMacro.z);

    a = mod(a, sa);
    a = abs(a - 0.5 * sa);
    vec2 q = r * vec2(cos(a), sin(a));

    // bass deformation: radial ripples
    q *= 1.0 + 0.28 * uBass * sin(r * 7.0 - uTime * 2.2);

    // folding geometry (IFS)
    vec2  z   = q * 2.4;
    float d   = 1e3;
    float acc = 0.0;
    int iters = 3 + int(uMacro.y * 6.0);
    for (int i = 0; i < 9; ++i)
    {
        if (i >= iters) break;
        float fi = float(i);
        z = abs(z) - vec2(0.55 + 0.18 * sin(uTime * 0.21 + fi * 1.3), 0.32 + 0.1 * uMid);
        z *= rot(0.55 + uTime * 0.045 + 0.25 * uMid);
        z *= 1.28;
        float l = length(z);
        d = min(d, abs(l - 0.45) / pow(1.28, fi + 1.0));
        acc += exp(-l * 2.5);
    }

    float lines = smoothstepR(0.010, 0.0, d) * 0.85 + 0.0022 / (d + 0.006);
    float t = r * 0.9 + acc * 0.12 + uTime * 0.025 + uCentroid * 0.35;

    vec3 col = palette(t) * lines * (0.35 + 0.9 * uIntensity);
    col += palette(t + 0.33) * 0.35 * exp(-r * 3.5) * (0.3 + 2.0 * uKick);

    col *= 0.25 + 0.75 * smoothstepR(1.05, 0.25, r);

    // transient shock ring travelling outwards over one beat
    float ringR = beatEase(uBeatPhase) * 0.9;
    col += palette(t + 0.55) * uTransient * 1.2 * exp(-abs(r - ringR) * 60.0);

    // high-frequency sparkle on the fold lines
    col += vec3(uHigh * 0.6) * exp(-d * 400.0) * hash12(floor(gl_FragCoord.xy * 0.5) + floor(uAbsTime * 30.0));

    // mirror feedback: previous frame, mirrored, slightly zoomed & rotated
    vec2 f = screenUV() - 0.5;
    f *= 0.965 - 0.03 * uBass;
    f  = rot(0.012 * (uMacro.z - 0.5) * 4.0) * f;
    vec3 prev  = texture(uPrev, 0.5 + f).rgb;
    vec3 prevM = texture(uPrev, 0.5 + vec2(-f.x, f.y)).rgb;
    vec3 fb = mix(prev, prevM, 0.5) * uMacro.w * 0.93;

    col = max(col, fb);
    fragColor = vec4(safeHDR(col), 1.0);
}
