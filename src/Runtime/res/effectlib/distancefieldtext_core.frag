in vec2 sampleCoord;

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;

in vec2 alphas;

void main()
{
    fragColor = color * smoothstep(alphas.x, alphas.y,
                                   texture(_qt_texture, sampleCoord).r);
}
