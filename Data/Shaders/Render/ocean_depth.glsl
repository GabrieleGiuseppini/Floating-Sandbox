###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inOceanAttribute;	// Position (vec2), Depth (float)

// Parameters
uniform float paramAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanDepthColorStart;
uniform vec3 paramOceanDepthColorEnd;
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 oceanColor;

void main()
{
    // Calculate color
    vec3 oceanColorTmp = paramOceanDepthColorStart * (1 - inOceanAttribute.z)
        + paramOceanDepthColorEnd * inOceanAttribute.z;
    oceanColor = vec4(oceanColorTmp.xyz * paramAmbientLightIntensity, 1.0 - paramOceanTransparency);

    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inOceanAttribute.xy, -1.0, 1.0);
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
