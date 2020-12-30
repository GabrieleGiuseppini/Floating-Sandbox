###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inOceanDetailed1;	// Position (vec2)
in vec4 inOceanDetailed2;	// yWater, yBack, yMid, yFront (float: 0.0 at top, +foo at bottom)

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanFlatColor;
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 oceanColor;
out vec4 yWaters;

void main()
{
    // Calculate color
    oceanColor = vec4(paramOceanFlatColor.xyz, 1.0 - paramOceanTransparency);

    // Apply ambient light
    oceanColor = oceanColor * paramEffectiveAmbientLightIntensity;

    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1, -1.0, 1.0);

    // Pass y values to fragment shader
    yWaters = inOceanDetailed2;
}


###FRAGMENT

#version 120

#include "ocean_surface.glslinc"

#define in varying

// Inputs from previous shader
in vec4 oceanColor;
in vec4 yWaters;

void main()
{
    vec4 color = CalculateOceanPlaneColor(oceanColor.xyz, yWaters.x, yWaters.y, yWaters.z, yWaters.w, 1.);
    gl_FragColor = vec4(color.xyz, color.w * oceanColor.w);
} 
