// 06 — LIQUID DREAM
// Macros: A Wave Scale · B Refraction · C Viscosity · D Dream Feedback
float heightField(vec2 p, float t)
{
    float h = 0.0;
    float amp = 0.18 * (0.5 + 1.4 * uBass);
    for (int i = 0; i < 5; ++i)
    {
        float fi = float(i);
        vec2  dir = vec2(cos(fi * 1.7 + 0.3), sin(fi * 1.7 + 0.3));
        float fr  = (1.5 + fi * 0.9) * mix(0.5, 2.0, uMacro.x);
        h += sin(dot(p, dir) * fr + t * (0.8 + fi * 0.35)) * amp / (1.0 + fi * 0.6);
    }
    h += (fbm(p * 2.0 + t * 0.2, 4) - 0.5) * 0.35;
    return h;
}

void main()
{
    vec2  uv = centered();
    float t  = uTime * 0.35 * (0.25 + uMacro.z * 1.2);

    float e  = 2.0 / uRes.y;
    float h  = heightField(uv * 2.2, t);
    float hx = heightField((uv + vec2(e, 0.0)) * 2.2, t);
    float hy = heightField((uv + vec2(0.0, e)) * 2.2, t);
    vec3  n  = normalize(vec3((h - hx) / e, (h - hy) / e, 18.0));

    vec2  ruv = uv + n.xy * mix(0.02, 0.35, uMacro.y) * (1.0 + uKick);
    float bg  = fbm(ruv * 2.6 + vec2(t * 0.25, -t * 0.18), 5);
    float stripes = 0.5 + 0.5 * sin(bg * 42.0 + h * 6.0 + t * 1.5 + uMid * 3.0);

    vec3 col = palette(bg * 1.4 + h * 0.25 + uCentroid * 0.25) * (0.06 + 0.94 * stripes * stripes) * (0.6 + 0.8 * bg);

    // caustics
    float ca = pow(clamp(1.0 - abs(sin(h * 9.0 + bg * 4.0)), 0.0, 1.0), 6.0);
    col += palette(bg + 0.4) * ca * 0.4 * (0.4 + uHigh);

    // specular highlight
    vec3  L    = normalize(vec3(0.35, 0.5, 0.8));
    float spec = pow(max(dot(reflect(-L, n), vec3(0.0, 0.0, 1.0)), 0.0), 60.0);
    col += palette(0.85) * spec * (0.5 + 1.8 * uHigh);

    col *= 0.35 + 0.6 * uIntensity;
    col *= 1.0 + 0.5 * uKick;

    // dream feedback: refracted smear of the previous frame
    vec3 prev = texture(uPrev, screenUV() + n.xy * 0.004).rgb;
    float fbAmt = uMacro.w * 0.45 * (1.0 - 0.5 * uKick);
    col = mix(col, prev, fbAmt);

    fragColor = vec4(safeHDR(col), 1.0);
}
