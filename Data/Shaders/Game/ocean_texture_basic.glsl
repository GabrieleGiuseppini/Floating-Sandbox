###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inOceanBasic;	// Position (vec2), Texture coordinate Y (float) (0 at top, +X at bottom)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec3 textureCoord;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanBasic.xy, -1.0, 1.0);
    textureCoord = inOceanBasic;
}


###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "ocean.glslinc"

// Inputs from previous shader
in vec3 textureCoord;

// Input textures
uniform sampler2D paramOceanTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform float paramOceanTransparency;
uniform vec2 paramTextureScaling;
uniform float paramOceanDarkeningRate;

void main()
{
    // Back-sample 5.0 at top, slowly going to zero
    float sampleIncrement = -5.0 * (2.0 - 2.0 * smoothstep(-1.0, 1.0, textureCoord.z));
    vec2 textureCoord2 = vec2(textureCoord.x, textureCoord.z + sampleIncrement);
    vec4 textureColor = texture2D(paramOceanTexture, textureCoord2 * paramTextureScaling);

    // Apply ambient light
    textureColor.xyz = ApplyAmbientLight(textureColor.xyz, paramEffectiveMoonlightColor, paramEffectiveAmbientLightIntensity);

    // Apply depth darkening
    textureColor.xyz = ApplyDepthDarkening(
        textureColor.xyz,
        vec3(0.0),
        textureCoord.y,
        paramOceanDarkeningRate);

    // Lighten the top of the water
    textureColor *= 1.0 + (1.0 - smoothstep(0.0, 1.0, textureCoord.z)) * 0.1;

    gl_FragColor = vec4(textureColor.xyz, textureColor.w * (1.0 - paramOceanTransparency));
} 
