// 02 — ORGANIC FLUX
// Macros: A Warp Depth · B Detail · C Flow · D Folds
void main()
{
    float t = uTime * 0.12 * (0.4 + uMacro.z * 1.2);
    vec2  p = centered() * (1.7 - 0.45 * uBass - 0.15 * uKick);
    p += vec2(uPan * 0.1, 0.0);

    vec2 q = vec2(fbm(p + vec2(0.0, t), 5),
                  fbm(p + vec2(5.2, 1.3) - t, 5));

    float w = mix(1.5, 5.5, uMacro.x) * (1.0 + 0.9 * uBass);
    vec2 r = vec2(fbm(p + w * q + vec2(1.7, 9.2) + 0.15 * t, 5),
                  fbm(p + w * q + vec2(8.3, 2.8) + 0.126 * t, 5));

    int oct = 3 + int(uMacro.y * 3.0 + uHigh * 2.0);
    float f = fbm(p + w * r, oct);

    // high-frequency micro detail
    f += (fbm(p * 9.0 + r * 4.0 + t * 3.0, 3) - 0.5) * 0.35 * uHigh;

    float folds = mix(1.0, 7.0, uMacro.w);
    float band  = 0.5 + 0.5 * sin(f * TAU * folds + t * 2.0 + uMid * 2.0);

    vec3 col = palette(f * 1.4 + length(q) * 0.45 + uCentroid * 0.25);
    col *= mix(0.12, 1.0, band) * (0.25 + f * 1.35);

    // relief lighting from the field gradient
    vec3 n = normalize(vec3(dFdx(f), dFdy(f), 0.0035));
    float sh = clamp(dot(n, normalize(vec3(-0.5, 0.7, 0.45))), 0.0, 1.0);
    col += palette(f + 0.45) * pow(sh, 10.0) * (0.25 + 1.2 * uMid);

    col *= 0.45 + 0.9 * uIntensity;
    col *= 1.0 + 0.6 * uKick + 0.4 * uTransient;

    float v = length(centered());
    col *= smoothstepR(1.1, 0.2, v);
    fragColor = vec4(safeHDR(col), 1.0);
}
