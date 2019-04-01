in vec3 vCoord;
in vec2 tCoord;
in vec4 textureBounds;

out vec2 sampleCoord;
out vec2 shadowSampleCoord;

out vec2 alphas;
out vec4 normalizedTextureBounds;

uniform mat4 mvp;
uniform mat4 modelView;
uniform int textureWidth;
uniform int textureHeight;
uniform float fontScale;
uniform vec2 shadowOffset;

float thresholdFunc(float scale)
{
    float base = 0.5;
    float baseDev = 0.065;
    float devScaleMin = 0.15;
    float devScaleMax = 0.3;
    return base - ((clamp(scale, devScaleMin, devScaleMax) - devScaleMin)
                   / (devScaleMax - devScaleMin) * -baseDev + baseDev);
}

float spreadFunc(float scale)
{
    return 0.06 / scale;
}

vec2 alphaRange(float scale)
{
    float base = thresholdFunc(scale);
    float range = spreadFunc(scale);
    float alphaMin = max(0.0, base - range);
    float alphaMax = min(base + range, 1.0);
    return vec2(alphaMin, alphaMax);
}

void main()
{
     float scale = fontScale * sqrt(abs(determinant(modelView)));
     alphas = alphaRange(scale);

     vec2 textureSizeMultiplier = vec2(1.0 / textureWidth, 1.0 / textureHeight);

     sampleCoord = tCoord * textureSizeMultiplier;
     shadowSampleCoord = (tCoord - shadowOffset) * textureSizeMultiplier;
     normalizedTextureBounds = vec4(textureBounds.xy * textureSizeMultiplier,
                                    textureBounds.zw * textureSizeMultiplier);
     gl_Position = mvp * vec4(vCoord, 1.0);
}
