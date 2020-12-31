###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inOceanDetailed1;	// Position (vec2)
in vec4 inOceanDetailed2;	// yTexture (float, UNUSUED), yBack/yMid/yFront (float, world y)

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanFlatColor;
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 oceanColor;
out vec4 yVector;

void main()
{
    // Calculate color
    oceanColor = vec4(paramOceanFlatColor.xyz, 1.0 - paramOceanTransparency);

    // Apply ambient light
    oceanColor = oceanColor * paramEffectiveAmbientLightIntensity;

    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1, -1.0, 1.0);

    // Pass y values to fragment shader
    yVector = vec4(inOceanDetailed1.y, inOceanDetailed2.yzw);
}


###FRAGMENT

#version 120

#include "ocean_surface.glslinc"

#define in varying

// Inputs from previous shader
in vec4 oceanColor;
in vec4 yVector;

// Parameters
uniform float paramOceanSurfaceBackPlaneToggle;

void main()
{
    gl_FragColor = CalculateOceanPlaneColor(oceanColor, yVector.x, yVector.y, yVector.z, yVector.w, paramOceanSurfaceBackPlaneToggle);
}
