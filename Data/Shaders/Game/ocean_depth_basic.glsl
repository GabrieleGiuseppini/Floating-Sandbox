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

#include "ocean.glslinc"

// Inputs from previous shader
in float oceanWorldY;

// Input textures
uniform sampler2D paramNoiseTexture;

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanDepthColorStart;
uniform vec3 paramOceanDepthColorEnd;
uniform float paramOceanDarkeningRate;

void main()
{
    vec3 oceanColor = ApplyDepthDarkeningWithDither(
        paramOceanDepthColorStart,
        paramOceanDepthColorEnd,
        oceanWorldY,
        paramOceanDarkeningRate,
        gl_FragCoord.xy,
        paramNoiseTexture);

    gl_FragColor = vec4(oceanColor.xyz * paramEffectiveAmbientLightIntensity, 1.0 - paramOceanTransparency);
} 
