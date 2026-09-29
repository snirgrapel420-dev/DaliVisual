// Scene transition: uTex = outgoing scene, uLayer = incoming scene.
uniform sampler2D uLayer;
uniform float uMix;
void main()
{
    vec2 uv = screenUV();
    vec3 a = texture(uTex, uv).rgb;
    vec3 b = texture(uLayer, uv).rgb;
    // luminance-keyed dissolve: bright structures of the new scene arrive first
    float k = clamp(uMix * 1.6 - 0.3 + dot(b, vec3(0.333)) * 0.3 * (1.0 - uMix), 0.0, 1.0);
    fragColor = vec4(mix(a, b, smoothstep(0.0, 1.0, k)), 1.0);
}
