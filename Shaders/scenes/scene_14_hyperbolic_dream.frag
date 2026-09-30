// 14 HYPERBOLIC DREAM — an endless {p,q} tiling of the hyperbolic plane (Poincare disk).
//   bass clock  slides the whole tiling through hyperbolic space (Mobius translation)
//   spectrum    every tile generation glows with its own frequency band
//   kick        tile edges flare · snare: the tiling rotates a step · hi-hat: edge sparkle
//   outside the disk the plane is mirrored back in, so the whole screen is tiled
// Macros: A Tiling Type · B Edge Width · C Flow Speed · D Depth Glow

vec2 cmul(vec2 a, vec2 b) { return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x); }
vec2 cdiv(vec2 a, vec2 b) { return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / dot(b, b); }

vec3 tile(vec2 z)
{
    // tiling type {p,q}
    float sel = floor(uMacro.x * 4.99);
    float P = 7.0, Q = 3.0;
    if (sel > 0.5) { P = 5.0; Q = 4.0; }
    if (sel > 1.5) { P = 6.0; Q = 4.0; }
    if (sel > 2.5) { P = 8.0; Q = 3.0; }
    if (sel > 3.5) { P = 4.0; Q = 5.0; }
    float sp = sin(PI / P), cq = cos(PI / Q);
    float k = sqrt(cq * cq - sp * sp);
    float R = sp / k, D = cq / k;

    // outside the unit disk: mirror back in (fills the screen)
    float outside = 0.0;
    if (dot(z, z) > 1.0) { z /= dot(z, z); outside = 1.0; }
    float scale0 = 1.0 - dot(z, z);

    // Mobius translation by the bass clock: the tiling flows through the plane
    float flow = uBassTime * (0.08 + 0.35 * uMacro.z);
    vec2 a = 0.55 * vec2(cos(flow * 0.7), sin(flow * 0.53)) * (0.6 + 0.4 * sin(flow * 0.21));
    z = cdiv(z + a, vec2(1.0, 0.0) + cmul(vec2(a.x, -a.y), z));
    z = cmul(z, vec2(cos(0.3 * uSnare + 0.02 * uMidTime), sin(0.3 * uSnare + 0.02 * uMidTime)));

    // fold into the fundamental triangle
    float count = 0.0;
    float sector = PI / P;
    for (int i = 0; i < 40; i++)
    {
        float ang = atan(z.y, z.x);
        float a2 = mod(ang, 2.0 * sector);
        if (a2 > sector) { a2 = 2.0 * sector - a2; count += 1.0; }
        z = length(z) * vec2(cos(a2), sin(a2));
        vec2 dz = z - vec2(D, 0.0);
        float d2 = dot(dz, dz);
        if (d2 < R * R) { z = vec2(D, 0.0) + dz * (R * R / d2); count += 1.0; }
        else break;
    }

    // distance to the triangle's edges (in the folded domain)
    float eCirc = abs(length(z - vec2(D, 0.0)) - R);
    float eLine = abs(z.y);
    float eRay  = abs(dot(z, vec2(-sin(sector), cos(sector))));
    float edge = min(eCirc, min(eLine, eRay));
    float scale = 1.0 - dot(z, z);                                      // hyperbolic shrink toward the rim

    float gen = count;
    float band = spec(fract(gen * 0.061 + 0.03) * 0.9 + 0.03);
    vec3 fill = palette(gen * 0.055 + length(z) * 0.4 + uPalShift + 0.015 * uMidTime + outside * 0.5);
    vec3 col = fill * (0.12 + 0.45 * band + 1.4 * band * band * band) * (0.55 + 0.45 * smoothstep(0.0, 0.6, length(z)));

    float w = (0.006 + 0.02 * uMacro.y) * (1.0 + 1.5 * uKick);
    float line = smoothstep(w, w * 0.3, edge);
    float rimFade = smoothstep(0.0, 0.12, scale0);                        // lines thinner than a pixel fade out
    col += palette(gen * 0.055 + 0.5 + uPalShift) * line * rimFade * (0.35 + 1.4 * uKick + 0.8 * uHat);
    col += palette(0.6 + uPalShift) * exp(-edge * 60.0) * 0.15 * (0.5 + uHigh);
    // depth glow toward the rim
    col *= mix(1.0, 0.35 + 0.65 * smoothstep(0.0, 0.2 + 0.4 * uMacro.w, count * 0.08), 0.6);
    return col * (1.0 - 0.45 * outside);
}

void main()
{
    vec3 col = vec3(0.0);
    float zoom = 0.72 * (1.0 - 0.06 * uKick);
    for (int s = 0; s < 4; s++)
    {
        vec2 o = (vec2(float(s & 1), float(s >> 1)) - 0.5) * 0.5;
        vec2 p = (gl_FragCoord.xy + o - 0.5 * uRes) / (0.5 * uRes.y) * zoom;
        col += tile(p);
    }
    col *= 0.25 * uIntensity * 1.25;
    fragColor = vec4(col, 1.0);
}
