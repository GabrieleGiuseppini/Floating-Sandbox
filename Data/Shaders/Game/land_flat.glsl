###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inLand; // Position

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out float yWorld;

void main()
{
    yWorld = inLand.y;

    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inLand.xy, -1.0, 1.0);
}

###FRAGMENT-120

#include "land.glslinc"

#define in varying

// Inputs from previous shader
in float yWorld;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform vec3 paramLandFlatColor;
uniform float paramOceanDarkeningRate;

void main()
{
    // Apply depth darkening
    vec3 color = ApplyDepthDarkening(
        paramLandFlatColor,
        vec3(0.),
        yWorld,
        paramOceanDarkeningRate);

    // Apply ambient light
    gl_FragColor = vec4(
        ApplyAmbientLight(color, paramEffectiveMoonlightColor, paramEffectiveAmbientLightIntensity),
        1.0);
} 
