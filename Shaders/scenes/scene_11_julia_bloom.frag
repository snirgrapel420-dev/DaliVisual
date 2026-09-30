// 11 JULIA BLOOM — a living Julia fractal.
//   bass clock   morphs the fractal (c travels around the Mandelbrot boundary)
//   kick         zoom punch · snare: shape jump · mids: slow rotation
//   spectrum     every escape contour glows with its own frequency band
// Macros: A Symmetry Fold · B Detail · C Morph Speed · D Glow

vec3 julia(vec2 p)
{
    // optional kaleidoscopic fold before iterating
    float folds = floor(1.0 + max(0.0, uMacro.x - 0.5) * 12.0);      // off at the default, up to 7-fold
    if (folds > 1.5)
    {
        float a = atan(p.y, p.x), r = length(p);
        float seg = TAU / folds;
        a = abs(mod(a, seg) - 0.5 * seg);
        p = r * vec2(cos(a), sin(a));
    }
    float m = uBassTime * (0.05 + 0.2 * uMacro.z) + 0.35 * uSnare;
    // c travels just inside the main cardioid boundary: always a connected, spiral-rich Julia set
    float th = m + 1.2;
    vec2 e1 = vec2(cos(th), sin(th)), e2 = vec2(cos(2.0 * th), sin(2.0 * th));
    vec2 c = (0.5 * e1 - 0.25 * e2) * (0.9985 - 0.006 * uBass) + 0.004 * vec2(sin(uMidTime * 0.3), cos(uMidTime * 0.21));
    vec2 z = p;
    float trap = 1e9;
    vec2 zTrap = vec2(0.0);
    int maxIt = 60 + int(uMacro.y * 80.0);
    int i = 0;
    for (int k = 0; k < 140; k++)
    {
        if (k >= maxIt) break;
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        float tr = abs(length(z) - 0.5 - 0.2 * uBass);                  // ring trap breathes with the bass
        if (tr < trap) { trap = tr; zTrap = z; }
        if (dot(z, z) > 256.0) break;
        i++;
    }
    vec3 col;
    if (i >= maxIt)
    {
        // interior: swirling glass coloured by the orbit angle, lit by the mid band
        float ang = atan(zTrap.y, zTrap.x) / TAU;
        float bandI = spec(0.35 + 0.3 * fract(ang * 2.0));
        col = palette(ang * 2.0 + log(trap + 1e-3) * 0.12 + uPalShift + 0.02 * uMidTime);
        col *= (0.12 + 0.9 * exp(-trap * 5.0)) * (0.35 + 1.1 * bandI);
    }
    else
    {
        float n = float(i) - log2(log2(dot(z, z))) + 4.0;                // smooth escape count
        float band = spec(fract(n * 0.06));                             // each contour = a frequency
        col = palette(n * 0.07 + uPalShift + 0.01 * uMidTime);
        col *= 0.08 + 1.4 * pow(band, 1.5) + 0.25 * sin(n * 0.35 - uMidTime * 1.5) * 0.5 + 0.1;
        col += palette(n * 0.07 + 0.5 + uPalShift) * exp(-trap * 25.0) * (0.4 + 1.5 * uMacro.w) * (0.4 + uHigh);
    }
    return col;
}

void main()
{
    vec3 col = vec3(0.0);
    float zoom = 0.5 * (1.0 - 0.12 * uKick) * (1.0 + 0.1 * sin(uMidTime * 0.07));
    mat2 R = rot(uMidTime * 0.03);
    for (int s = 0; s < 4; s++)
    {
        vec2 o = vec2(float(s & 1), float(s >> 1)) - 0.5;
        vec2 p = (gl_FragCoord.xy + o * 0.5 - 0.5 * uRes) / uRes.y;
        col += julia(R * p * zoom * 2.0);
    }
    col *= 0.25 * uIntensity * 1.2;
    fragColor = vec4(col, 1.0);
}
