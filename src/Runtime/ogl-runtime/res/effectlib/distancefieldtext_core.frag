in vec2 sampleCoord;

out vec4 fragColor;

uniform sampler2D _qt_texture;
uniform vec4 color;

void main()
{
    float distance = texture(_qt_texture, sampleCoord).r;
    float f = fwidth(distance);
    fragColor = color * smoothstep(0.5 - f, 0.5, distance);
}
