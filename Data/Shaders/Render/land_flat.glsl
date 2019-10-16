###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inLand; // Position

// Parameters
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramLandFlatColor;
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 landColor;

void main()
{
    // Calculate color
    landColor = vec4(paramLandFlatColor * paramEffectiveAmbientLightIntensity, 1.0);

    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inLand.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec4 landColor;

void main()
{
    gl_FragColor = landColor;
} 
