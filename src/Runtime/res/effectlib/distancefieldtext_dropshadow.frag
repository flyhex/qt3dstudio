varying highp vec2 sampleCoord;
varying highp vec2 alphas;
varying highp vec2 shadowSampleCoord;
varying highp vec4 normalizedTextureBounds;

uniform sampler2D _qt_texture;
uniform highp vec4 color;
uniform highp vec4 shadowColor;

void main()
{
    highp float shadowAlpha = smoothstep(alphas.x,
                                         alphas.y,
                                         texture2D(_qt_texture,
                                                   clamp(shadowSampleCoord,
                                                         normalizedTextureBounds.xy,
                                                         normalizedTextureBounds.zw)).a);
    highp vec4 shadowPixel = color * shadowColor * shadowAlpha;

    highp float textAlpha = smoothstep(alphas.x,
                                       alphas.y,
                                       texture2D(_qt_texture,
                                                 clamp(sampleCoord,
                                                       normalizedTextureBounds.xy,
                                                       normalizedTextureBounds.zw)).a);
    highp vec4 textPixel = color * textAlpha;

    gl_FragColor = mix(shadowPixel, textPixel, textPixel.a);
}
