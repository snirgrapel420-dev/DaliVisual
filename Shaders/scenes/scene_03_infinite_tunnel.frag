// 03 — INFINITE TUNNEL
// Macros: A Shape · B Wall Pattern · C Travel Speed · D Trails
void main()
{
    vec2 p = centered();
    p += 0.09 * vec2(sin(uTime * 0.31), cos(uTime * 0.23)) * (0.4 + uWidth);
    p.x += uPan * 0.1;

    float a  = atan(p.y, p.x);
    float rc = length(p);

    // circle -> hexagon morph
    float n  = 6.0;
    float an = mod(a + PI / n, TAU / n) - PI / n;
    float rp = rc * cos(an) / cos(PI / n);
    float r  = mix(rc, rp, uMacro.x);

    // frequency deformation of the walls
    r *= 1.0 - 0.12 * uBass * sin(a * 3.0 + uTime * 1.3);
    r += 0.03 * uMid  * sin(a * 8.0  + uTime * 2.0);
    r += 0.012 * uHigh * sin(a * 24.0 - uTime * 5.0);
    r  = max(r, 1e-3);

    float z = 0.55 / r;
    float travel = uTime * (0.4 + uMacro.z * 2.2) + beatTravel() * (0.25 + 0.5 * uMacro.z);

    float segs  = 12.0;
    float twist = uMacro.y;
    vec2  w     = vec2(a / TAU * segs + z * twist * 0.35, (z + travel) * 1.5);

    // wall tiles: bevelled panels with glowing seams
    vec2  id = floor(w);
    vec2  tf = fract(w) - 0.5;
    float seam  = 0.5 - max(abs(tf.x), abs(tf.y));
    float glowL = exp(-seam * 28.0);
    float panel = smoothstep(0.0, 0.12, seam);
    float hsh   = hash12(id);
    float lit   = step(0.86 - 0.25 * uMid, hash12(id + floor(uBeatClock))) ;

    float depthT = w.y * 0.05;
    vec3 col = palette(depthT + hsh * 0.15 + uCentroid * 0.3) * glowL * (0.7 + 2.0 * uKick)
             + palette(depthT + 0.35) * panel * (0.05 + 0.15 * hsh + 0.5 * lit * uBeat)
             + palette(depthT + 0.6) * panel * lit * 0.25;

    // depth fog, bright core
    float fade = 1.0 - exp(-r * 6.0);
    col *= fade * (0.45 + 0.9 * uIntensity);
    col += palette(travel * 0.04 + 0.5) * 0.45 * exp(-rc * 9.0) * (0.3 + 1.5 * uEnergy);
    col += palette(0.8) * uTransient * 0.5 * glowL;

    // feedback trails expanding towards the viewer
    vec2 f = screenUV() - 0.5;
    vec3 prev = texture(uPrev, 0.5 + f * (0.975 - 0.02 * uBass)).rgb;
    col = max(col, prev * uMacro.w * 0.92);

    fragColor = vec4(safeHDR(col), 1.0);
}
