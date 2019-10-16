###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inOcean;	// Position (vec2), IGNORED (float)

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanFlatColor;
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 oceanColor;

void main()
{
    // Calculate color
    oceanColor = vec4(paramOceanFlatColor.xyz, 1.0 - paramOceanTransparency);

    // Apply ambient light
    oceanColor = oceanColor * paramEffectiveAmbientLightIntensity;

    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inOcean.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec4 oceanColor;

void main()
{
    gl_FragColor = oceanColor;
} 
