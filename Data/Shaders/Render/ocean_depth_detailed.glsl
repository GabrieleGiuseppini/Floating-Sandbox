###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inOceanDetailed1;	// Position (vec2)
in vec4 inOceanDetailed2;	// yTexture (float, UNUSUED), yBack/yMid/yFront (float, world y)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 yVector;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1, -1.0, 1.0);
    yVector = vec4(inOceanDetailed1.y, inOceanDetailed2.yzw);
}


###FRAGMENT

#version 120

#include "ocean_surface.glslinc"

#define in varying

// Inputs from previous shader
in vec4 yVector;

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanDepthColorStart;
uniform vec3 paramOceanDepthColorEnd;
uniform float paramOceanDarkeningRate;
uniform float paramOceanSurfaceBackPlaneToggle;

void main()
{
    // Get depth color sample
    float darkMix = 1.0 - exp(min(0.0, yVector.x) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec3 depthColor = mix(
        paramOceanDepthColorStart,
        paramOceanDepthColorEnd, 
        pow(darkMix, 3.0));

    // Apply detail
    vec4 color = CalculateOceanPlaneColor(
        vec4(depthColor, (1.0 - paramOceanTransparency)),
        yVector.x, 
        yVector.y, yVector.z, yVector.w, 
        paramOceanSurfaceBackPlaneToggle);

    // Combine
    gl_FragColor = vec4(color.xyz * paramEffectiveAmbientLightIntensity, color.w);
} 
