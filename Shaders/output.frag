// Final output: global colour grade + soft highlight roll-off + dither.
uniform float uHue, uSaturation, uBrightness, uContrast;

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
    vec3 c = texture(uTex, vUV).rgb;
    c = hueRotate(c, uHue);
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, uSaturation);
    c *= uBrightness;
    c = (c - 0.5) * uContrast + 0.5;
    c = softClip(max(c, 0.0));
    c += (hash12(gl_FragCoord.xy + fract(uAbsTime) * 91.0) - 0.5) / 255.0;
    fragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
