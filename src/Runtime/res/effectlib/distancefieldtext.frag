varying highp vec2 sampleCoord;
varying highp vec2 alphas;

uniform sampler2D _qt_texture;
uniform highp vec4 color;

void main()
{
    gl_FragColor = color * smoothstep(alphas.x,
                                      alphas.y,
                                      texture2D(_qt_texture, sampleCoord).a);
}
