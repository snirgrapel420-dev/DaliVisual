// 07 — NEURAL BLOOM
// Macros: A Density · B Connections · C Growth · D Particle Glow
vec2 nodePos(vec2 cell)
{
    vec2 o = hash22(cell);
    return 0.5 + 0.38 * sin(uTime * 0.35 + TAU * o);
}

float segDist(vec2 p, vec2 a, vec2 b, out float s)
{
    vec2 pa = p - a, ba = b - a;
    s = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * s);
}

void main()
{
    float dens = mix(3.0, 9.0, uMacro.x);
    vec2  p    = centered() * dens * (1.0 - 0.08 * uBass) + vec2(uTime * 0.04, uTime * 0.025);
    vec2  ip   = floor(p);
    vec2  fp   = p - ip;

    vec2  pts[9];
    vec2  cid[9];
    for (int j = 0; j < 3; ++j)
    for (int i = 0; i < 3; ++i)
    {
        vec2 g = vec2(float(i - 1), float(j - 1));
        cid[j * 3 + i] = ip + g;
        pts[j * 3 + i] = g + nodePos(ip + g);
    }

    float px   = dens / uRes.y;                 // one pixel in cell units
    float grow = clamp(uMacro.z + 0.35 * uBass, 0.0, 1.0);
    float conn = 0.0, pulse = 0.0, node = 0.0, memb = 1e3;

    for (int k = 0; k < 9; ++k)
    {
        float d = length(fp - pts[k]);
        node += exp(-d * d * mix(140.0, 45.0, uKick)) * (0.6 + 2.5 * uTransient);
        memb  = min(memb, d);

        int ix = k - (k / 3) * 3;
        int iy = k / 3;
        for (int e = 0; e < 3; ++e)
        {
            int nx = ix + (e == 0 ? 1 : (e == 1 ? 0 : 1));
            int ny = iy + (e == 0 ? 0 : 1);
            if (nx > 2 || ny > 2) continue;
            int kk = ny * 3 + nx;
            float hsh = hash12(cid[k] * 1.37 + cid[kk] * 0.71);
            if (hsh > mix(0.25, 0.95, uMacro.y)) continue;
            // grow the connection from its source node
            float lenGrow = clamp(grow * 1.6 - hsh, 0.0, 1.0);
            vec2 b = mix(pts[k], pts[kk], lenGrow);
            float s;
            float sd = segDist(fp, pts[k], b, s);
            float w  = exp(-sd / (px * 1.4)) * 0.8;
            conn += w;
            float ph = fract(s * 0.8 - uTime * 0.7 - hsh * 3.0 + uBeatPhase * 0.5);
            pulse += w * exp(-abs(ph - 0.5) * 25.0) * (0.4 + 2.0 * uMid);
        }
    }

    // membranes between cells (faint)
    float cellEdge = smoothstep(0.45, 0.5, memb) * 0.06;

    // particles / sparks
    vec2  sp  = centered() * 90.0 + vec2(0.0, -uTime * 3.0);
    float spk = step(0.985 - 0.02 * uHigh, hash12(floor(sp) + floor(uAbsTime * 8.0)));
    float sparks = spk * exp(-length(fract(sp) - 0.5) * 6.0) * (0.2 + 2.0 * uHigh) * mix(0.3, 1.6, uMacro.w);

    vec3 col = palette(0.55 + uCentroid * 0.3) * conn * 0.9
             + palette(0.25) * pulse
             + palette(0.05 + uFlux * 0.3) * node * mix(0.6, 1.8, uMacro.w)
             + palette(0.7) * cellEdge
             + palette(0.9) * sparks;

    // transient burst: radial bloom from centre
    float r = length(centered());
    col += palette(0.15) * uTransient * 0.6 * exp(-abs(r - beatEase(uBeatPhase) * 0.7) * 25.0);

    col *= 0.5 + 0.8 * uIntensity;
    col *= smoothstepR(1.2, 0.3, r);
    fragColor = vec4(safeHDR(col), 1.0);
}
