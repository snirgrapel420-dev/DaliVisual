// 13 IRIDESCENT OIL — thin-film interference on a flowing, glossy liquid.
//   mid clock   stirs the flow · bass: the surface swells · kick: a ripple runs out from the centre
//   spectrum    changes the film thickness across the surface → the colours shift with the frequencies
//   hi-hat      specular glints sharpen
// Macros: A Flow Scale · B Film Thickness · C Turbulence · D Gloss

float fb(vec2 p) { return fbm(p, 4); }

float field(vec2 p)
{
    float t = uMidTime * 0.18;
    vec2 q = vec2(fb(p + vec2(0.0, t)), fb(p + vec2(5.2, 1.3) - t * 0.7));
    vec2 r = vec2(fb(p + (1.2 + 1.6 * uMacro.z) * q + vec2(1.7, 9.2) + t * 0.4),
                  fb(p + (1.2 + 1.6 * uMacro.z) * q + vec2(8.3, 2.8) - t * 0.3));
    float h = fb(p + (1.6 + 1.2 * uBass) * r);
    // kick ripple
    float rr = length(p - vec2(1.6, 0.9));
    h += 0.08 * uKick * sin(rr * 22.0 - (1.0 - uKick) * 14.0) * exp(-rr * 0.8);
    return h;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uRes.y;
    vec2 p = uv * (0.7 + 1.1 * uMacro.x);

    float e = 2.0 / uRes.y;
    float h  = field(p);
    float hx = field(p + vec2(e, 0.0));
    float hy = field(p + vec2(0.0, e));
    vec3 n = normalize(vec3((h - hx) * 1.6, (h - hy) * 1.6, e * 6.0 / (0.7 + 1.1 * uMacro.x)));

    // film thickness: flow height + the spectrum at this screen column (low left .. high right)
    float fx = clamp(gl_FragCoord.x / uRes.x, 0.0, 1.0);
    float thick = h * (0.7 + 1.4 * uMacro.y) + 0.45 * specBand(0.02 + fx * 0.86, 0.12 + fx * 0.86) + 0.15 * uBass;
    vec3 film = 0.5 + 0.5 * cos(TAU * (thick * vec3(1.0, 1.14, 1.31) + vec3(0.0, 0.1, 0.2)));
    vec3 tint = palette(h * 0.6 + uPalShift + 0.02 * uMidTime);
    vec3 base = mix(film, film * tint * 1.7, 0.6);

    // glossy light: two coloured lights and a sharp specular that the hats sharpen
    vec3 v = vec3(0.0, 0.0, 1.0);
    vec3 l1 = normalize(vec3(0.6, 0.5, 0.7)), l2 = normalize(vec3(-0.7, -0.2, 0.5));
    float d1 = max(dot(n, l1), 0.0), d2 = max(dot(n, l2), 0.0);
    float shin = 30.0 + 120.0 * uMacro.w + 200.0 * uHat;
    float s1 = pow(max(dot(reflect(-l1, n), v), 0.0), shin);
    float s2 = pow(max(dot(reflect(-l2, n), v), 0.0), shin * 0.5);
    vec3 col = base * (0.25 + 0.75 * d1 + 0.3 * d2);
    col += (s1 * 1.4 + s2 * 0.6 * palette(0.6 + uPalShift)) * (0.6 + 0.8 * uHigh);
    // valleys darken into deep oil
    col *= 0.04 + 1.15 * pow(smoothstep(0.38, 0.74, h), 1.6);

    col *= uIntensity * 1.15 * mix(0.55, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
