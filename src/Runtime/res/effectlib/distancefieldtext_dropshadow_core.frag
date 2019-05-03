in vec2 sampleCoord;
in vec2 shadowSampleCoord;
in vec4 normalizedTextureBounds;

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;
uniform vec4 shadowColor;

in vec2 alphas;

void main()
{
    float shadowAlpha = smoothstep(alphas.x,
                                   alphas.y,
                                   texture(_qt_texture,
                                           clamp(shadowSampleCoord,
                                                 normalizedTextureBounds.xy,
                                                 normalizedTextureBounds.zw)).r);
    vec4 shadowPixel = color * shadowColor * shadowAlpha;

    float textAlpha = smoothstep(alphas.x,
                                 alphas.y,
                                 texture(_qt_texture,
                                         clamp(sampleCoord,
                                               normalizedTextureBounds.xy,
                                               normalizedTextureBounds.zw)).r);
    vec4 textPixel = color * textAlpha;

    fragColor = mix(shadowPixel, textPixel, textPixel.a);
}
