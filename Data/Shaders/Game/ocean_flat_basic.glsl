###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inOceanBasic;	// Position (vec2), IGNORED (float)

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inOceanBasic.xy, -1.0, 1.0);
}


###FRAGMENT-120

#include "common.glslinc"
#include "ocean.glslinc"

#define in varying

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform float paramOceanTransparency;
uniform vec3 paramOceanFlatColor;

void main()
{
    gl_FragColor = vec4(
        ApplyAmbientLight(paramOceanFlatColor, paramEffectiveMoonlightColor, paramEffectiveAmbientLightIntensity),
        1.0 - paramOceanTransparency);
} 
