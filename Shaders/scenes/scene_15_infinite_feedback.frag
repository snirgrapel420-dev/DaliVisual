// 15 INFINITE FEEDBACK — the classic video-feedback trip, fed only by the sound.
//   Every frame the previous frame is zoomed, turned, kaleidoscopically folded and colour-shifted,
//   then new light is injected from the audio: the spectrum as a ring, the waveform as a line,
//   the kick as a flash. Silence = the trails fade to black.
//   bass: warp strength · kick: zoom punch + flash · snare: rotation jolt · highs: colour drift
// Macros: A Symmetry · B Zoom · C Warp · D Trail Length

vec2 kaleido(vec2 p, float n)
{
    float a = atan(p.y, p.x), r = length(p);
    float seg = TAU / n;
    a = abs(mod(a + 0.5 * seg, seg) - 0.5 * seg);
    return r * vec2(cos(a), sin(a));
}

void main()
{
    float asp = uRes.x / uRes.y;
    vec2 p = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;

    // ---- feedback transform --------------------------------------------------------
    vec2 q = p;
    float sym = floor(3.0 + uMacro.x * 7.0);
    q = mix(q, kaleido(q, sym), 0.85);
    q *= (0.985 - 0.035 * uMacro.y) - 0.03 * uKick;                     // content streams outward
    q *= rot(0.006 + 0.012 * sin(uMidTime * 0.2) + 0.05 * uSnare);
    vec2 w = vec2(vnoise(p * 3.0 + uMidTime * 0.3), vnoise(p * 3.0 + 7.3 - uMidTime * 0.25)) - 0.5;
    q += w * (0.004 + 0.02 * uMacro.z) * (0.4 + 1.6 * uBass);
    vec2 uvPrev = vec2(q.x / asp, q.y) + 0.5;
    vec3 fb = texture(uPrev, uvPrev).rgb;
    fb = mix(fb, fb.gbr, 0.035 + 0.06 * uHigh);                          // hue drift in the trails
    fb *= (0.90 + 0.085 * uMacro.w) * mix(0.93, 1.0, uActivity);
    fb = max(fb - 0.002, 0.0);

    // ---- injection: the sound ---------------------------------------------------------
    vec3 inj = vec3(0.0);
    float r = length(p), a = atan(p.y, p.x);
    fb *= 1.0 - 0.06 * exp(-r * 6.0);                                   // the centre re-samples itself: extra decay there
    float x = abs(fract(a / TAU + 0.25) * 2.0 - 1.0);                    // mirrored angle → symmetric spectrum ring
    float band = spec(0.02 + x * 0.9);
    float ringR = 0.1 + 0.2 * band + 0.03 * uKick;
    float d = abs(r - ringR);
    inj += palette(x * 0.8 + uPalShift + 0.02 * uMidTime) * smoothstep(0.006, 0.0, d) * (0.15 + 0.9 * band);

    // waveform: a glowing line through the centre, turning slowly
    vec2 pw = rot(uMidTime * 0.05) * p;
    float wy = 0.12 * wave(pw.x * 0.6 + 0.5) * smoothstep(0.55, 0.2, abs(pw.x));
    inj += palette(0.55 + uPalShift) * smoothstep(0.004, 0.0, abs(pw.y - wy)) * 0.9 * uActivity;

    // kick flash in the centre, snare sparks around the ring
    inj += palette(0.1 + uPalShift) * exp(-r * 30.0) * 0.35 * uKick * uKick;
    float sp = step(0.985, fract(sin(floor(a * 30.0) * 77.3 + floor(uBeatClock * 2.0)) * 4375.5));
    inj += palette(0.8 + uPalShift) * sp * smoothstep(0.02, 0.0, abs(r - ringR - 0.05)) * 2.0 * uSnare;

    vec3 col = fb + inj * uIntensity * 1.3;
    col = col / (1.0 + 0.25 * max(col - 1.0, 0.0));                    // soft ceiling for the accumulation
    fragColor = vec4(min(col, vec3(3.0)), 1.0);
}
