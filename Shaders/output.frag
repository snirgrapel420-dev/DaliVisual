// Final output: musical dynamics (build tension / drop impact), global colour
// grade, soft highlight roll-off and dither.
uniform float uHue, uSaturation, uBrightness, uContrast;
uniform float uDynamics;     // 0..1 amount of build / drop treatment
uniform float uBloom;        // 0..1 bloom amount (uTex has mipmaps)

vec3 hueRotate(vec3 c, float h)
{
    const vec3 k = vec3(0.57735);
    float a = h * TAU, ca = cos(a);
    return c * ca + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - ca);
}

vec3 softClip(vec3 c)
{
    vec3 x = max(c - 0.8, 0.0);
    return min(c, vec3(0.8)) + 0.2 * (1.0 - exp(-x / 0.2));
}

void main()
{
    float build = uBuild * uDynamics;
    float drop  = uDrop * uDynamics;

    // drop: zoom punch (image jumps towards the viewer and settles)
    vec2 uv = (vUV - 0.5) * (1.0 - 0.06 * drop * drop) + 0.5;
    // build: slow breathing tunnel pull that tightens as tension grows
    uv = (uv - 0.5) * (1.0 - 0.025 * build * (0.5 + 0.5 * sin(uAbsTime * (2.0 + 6.0 * build)))) + 0.5;
    vec3 c = texture(uTex, uv).rgb;
    // bloom: wide, soft glow gathered from the mip chain (thresholded so darks stay deep)
    vec3 b1 = textureLod(uTex, uv, 2.0).rgb, b2 = textureLod(uTex, uv, 3.5).rgb,
         b3 = textureLod(uTex, uv, 5.0).rgb, b4 = textureLod(uTex, uv, 6.5).rgb;
    vec3 bloom = b1 * 0.30 + b2 * 0.30 + b3 * 0.25 + b4 * 0.15;
    bloom = max(bloom - 0.12, 0.0) * 1.6;
    c += bloom * uBloom * (1.0 + 0.8 * drop);

    c = hueRotate(c, uHue);
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    // tension drains colour and focuses the frame; the drop floods it back over-saturated
    float sat = uSaturation * (1.0 - 0.45 * build) * (1.0 + 0.35 * drop);
    c = mix(vec3(l), c, sat);
    c *= uBrightness * (1.0 + 0.55 * drop * drop);
    c = (c - 0.5) * uContrast * (1.0 + 0.25 * build) + 0.5;
    // build vignette closes in; drop adds a short white flash
    vec2 p = vUV - 0.5;
    c *= 1.0 - build * 0.55 * smoothstep(0.15, 0.75, length(p * vec2(uRes.x / uRes.y, 1.0)));
    c += vec3(0.12 * pow(drop, 4.0)) * (0.3 + l);
    c = aces(max(c, 0.0) * 1.1);
    c += (hash12(gl_FragCoord.xy + fract(uAbsTime) * 91.0) - 0.5) / 255.0;
    fragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
