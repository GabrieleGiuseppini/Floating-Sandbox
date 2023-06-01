###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inLand; // Position (vec2), Depth (float) (0 at top, +X at bottom)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec3 textureCoord;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inLand.xy, -1.0, 1.0);
    textureCoord = inLand;
}


###FRAGMENT-120

#include "land.glslinc"

#define in varying

// Inputs from previous shader
in vec3 textureCoord;

// The texture
uniform sampler2D paramLandTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform vec2 paramTextureScaling;
uniform float paramOceanDarkeningRate;

void main()
{
    // Back-sample 10.0 at top, slowly going to zero
    float sampleIncrement = -10.0 * (2.0 - 2.0 * smoothstep(-3.0, 3.0, textureCoord.z));
    vec2 textureCoord2 = vec2(textureCoord.x, -textureCoord.y + sampleIncrement);
    vec4 textureColor = texture2D(paramLandTexture, textureCoord2 * paramTextureScaling);

    // Apply depth darkening
    textureColor.xyz = ApplyDepthDarkening(
        textureColor.xyz,
        vec3(0.),
        textureCoord.y,
        paramOceanDarkeningRate);

    // Anti-aliasing
    float alpha = textureCoord.z / (0.2 + abs(dFdx(textureCoord.z)));

    // Apply ambient light and blend
    gl_FragColor = vec4(
        ApplyAmbientLight(textureColor.xyz, paramEffectiveMoonlightColor, paramEffectiveAmbientLightIntensity),
        textureColor.w * alpha);
} 
