###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inOceanDetailed1;	// Position (vec2)
in vec4 inOceanDetailed2;	// yWater, yBack, yMid, yFront (float: 0.0 at top, +foo at bottom)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 yWaters;
out float oceanWorldY;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1, -1.0, 1.0);
    yWaters = inOceanDetailed2;
    oceanWorldY = inOceanDetailed1.y;
}


###FRAGMENT

#version 120

#include "ocean_surface.glslinc"

#define in varying

// Inputs from previous shader
in vec4 yWaters;
in float oceanWorldY;

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
    float darkMix = 1.0 - exp(min(0.0, oceanWorldY) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec3 depthColor = mix(
        paramOceanDepthColorStart,
        paramOceanDepthColorEnd, 
        pow(darkMix, 3.0));

    // Apply detail
    vec4 color = CalculateOceanPlaneColor(
        vec4(depthColor, (1.0 - paramOceanTransparency)),
        yWaters.x, yWaters.y, yWaters.z, yWaters.w, paramOceanSurfaceBackPlaneToggle);

    // Combine
    gl_FragColor = vec4(color.xyz * paramEffectiveAmbientLightIntensity, color.w);
} 
