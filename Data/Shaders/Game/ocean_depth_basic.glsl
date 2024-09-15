###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inOceanBasic;	// Position (vec2), IGNORED (float)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out float oceanWorldY;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanBasic.xy, -1.0, 1.0);
    oceanWorldY = inOceanBasic.y;
}


###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"
#include "ocean.glslinc"

// Inputs from previous shader
in float oceanWorldY;

// Input textures
uniform sampler2D paramNoiseTexture;

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform float paramOceanTransparency;
uniform vec3 paramOceanDepthColorStart;
uniform vec3 paramOceanDepthColorEnd;
uniform float paramOceanDepthDarkeningRate;

void main()
{
    // Calculate depth darkening
    float darkeningFactor = CalculateOceanDepthDarkeningFactor(
        oceanWorldY,
        paramOceanDepthDarkeningRate);

    // Calculate lamp tool intensity
    float lampToolIntensity = CalculateLampToolIntensity(gl_FragCoord.xy);

    // Apply gradient
    vec3 oceanColor = mix(
        paramOceanDepthColorStart,
        paramOceanDepthColorEnd,
        darkeningFactor * (1.0 - lampToolIntensity));

    // Apply dither
    oceanColor = ApplyDither(
        oceanColor,
        gl_FragCoord.xy,
        paramNoiseTexture);

    // Apply ambient light
    oceanColor = ApplyAmbientLight(
        oceanColor, 
        paramEffectiveMoonlightColor * 0.5, 
        paramEffectiveAmbientLightIntensity,
        lampToolIntensity);

    gl_FragColor = vec4(oceanColor, 1.0 - paramOceanTransparency);
} 
