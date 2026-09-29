// 08 — PSYCHEDELIC VOID
// Macros: A Particle Density · B Nebula · C Depth Speed · D Chromatic
vec3 voidLayer(vec2 p)
{
    vec3 col = vec3(0.0);
    float travel = uTime * 0.07 * (0.4 + uMacro.z * 2.0) + beatTravel() * 0.04;

    // depth-sorted particle layers
    for (int i = 0; i < 6; ++i)
    {
        float fi = float(i);
        float z  = fract(fi / 6.0 + travel);
        float sc = mix(26.0, 0.8, z) * mix(0.6, 1.6, uMacro.x);
        float fade = smoothstep(0.0, 0.35, z) * smoothstepR(1.0, 0.8, z);
        vec2  q  = p * sc + fi * vec2(13.1, 7.7);
        vec2  id = floor(q);
        vec2  o  = hash22(id);
        float d  = length(fract(q) - o);
        float star = exp(-d * mix(28.0, 12.0, uHigh)) * step(0.45, hash12(id + 0.7));
        col += palette(o.x * 0.4 + fi * 0.08 + uCentroid * 0.2) * star * fade * (1.2 + 2.4 * uHigh);
    }

    // fractal nebula
    vec2 z = p * rot(uTime * 0.03) * 1.2;
    float acc = 0.0;
    for (int i = 0; i < 8; ++i)
    {
        z = abs(z) / max(dot(z, z), 1e-3) - vec2(0.72 + 0.05 * uBass, 0.61);
        acc += exp(-abs(length(z) - 1.0) * 6.0);
    }
    col += palette(0.75 + acc * 0.03) * acc * 0.11 * uMacro.y * (0.4 + uMid + uBass);

    // the void: dark core with a breathing rim
    float r = length(p);
    col *= 0.15 + 0.85 * smoothstep(0.02, 0.3 + 0.1 * uKick, r);
    col += palette(0.5) * exp(-abs(r - 0.28 - 0.12 * uKick) * 30.0) * (0.15 + 1.4 * uKick) * uIntensity;
    return col;
}

void main()
{
    vec2  p  = centered();
    p += vec2(uPan * 0.05, 0.0);
    float ca = mix(0.002, 0.03, uMacro.w) * (0.4 + 2.5 * uTransient + uBass);

    vec3 col;
    col.r = voidLayer(p * (1.0 + ca)).r;
    col.g = voidLayer(p).g;
    col.b = voidLayer(p * (1.0 - ca)).b;

    col *= 0.5 + 0.9 * uIntensity;
    col += palette(0.1) * uTransient * 0.08;

    vec3 prev = texture(uPrev, 0.5 + (screenUV() - 0.5) * 0.985).rgb;
    col = max(col, prev * 0.6 * uMacro.w);
    fragColor = vec4(safeHDR(col), 1.0);
}
