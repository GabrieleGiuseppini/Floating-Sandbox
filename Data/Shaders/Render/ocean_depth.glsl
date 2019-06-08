###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inOcean;	// Position (vec2), Depth (float)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec3 oceanCoord;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOcean.xy, -1.0, 1.0);
    oceanCoord = inOcean;
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec3 oceanCoord;

// Parameters
uniform float paramAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanDepthColorStart;
uniform vec3 paramOceanDepthColorEnd;
uniform float paramOceanDarkeningRate;

void main()
{
    float darkMix = 1.0 - exp(min(0.0, oceanCoord.y) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec3 oceanColor = mix(
        paramOceanDepthColorStart,
        paramOceanDepthColorEnd, 
        pow(darkMix, 3.0));

    gl_FragColor = vec4(oceanColor.xyz * paramAmbientLightIntensity, 1.0 - paramOceanTransparency);
} 
