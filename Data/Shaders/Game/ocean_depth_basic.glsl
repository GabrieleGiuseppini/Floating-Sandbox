###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inOceanBasic;	// Position (vec2), IGNORED (float)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out float oceanWorldY;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanBasic.xy, -1.0, 1.0);
    oceanWorldY = inOceanBasic.y;
}


###FRAGMENT-120

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
    // Get depth color sample
    float darkMix = 1.0 - exp(min(0.0, oceanWorldY) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec3 oceanColor = mix(
        paramOceanDepthColorStart,
        paramOceanDepthColorEnd, 
        darkMix * darkMix * darkMix);

    gl_FragColor = vec4(oceanColor.xyz * paramEffectiveAmbientLightIntensity, 1.0 - paramOceanTransparency);
} 
