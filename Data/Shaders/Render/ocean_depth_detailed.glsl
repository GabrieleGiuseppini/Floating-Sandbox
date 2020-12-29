###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inOceanDetailed1;	// TODO: Position (vec2), Depth (float)
in vec2 inOceanDetailed2;	// TODO

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out float oceanWorldY;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1.xy, -1.0, 1.0);
    oceanWorldY = inOceanDetailed1.y;
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in float oceanWorldY;

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec3 paramOceanDepthColorStart;
uniform vec3 paramOceanDepthColorEnd;
uniform float paramOceanDarkeningRate;

void main()
{
    float darkMix = 1.0 - exp(min(0.0, oceanWorldY) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec3 oceanColor = mix(
        paramOceanDepthColorStart,
        paramOceanDepthColorEnd, 
        pow(darkMix, 3.0));

    gl_FragColor = vec4(oceanColor.xyz * paramEffectiveAmbientLightIntensity, 1.0 - paramOceanTransparency);
} 
