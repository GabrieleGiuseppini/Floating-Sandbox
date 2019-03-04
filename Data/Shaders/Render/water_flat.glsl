###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inWaterAttribute;	// Position (vec2), PAD (float)

// Parameters
uniform float paramAmbientLightIntensity;
uniform float paramWaterTransparency;
uniform vec3 paramWaterFlatColor;
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 waterColor;

void main()
{
    // Calculate color
    waterColor = vec4(paramWaterFlatColor.xyz, 1.0 - paramWaterTransparency);

    // Apply ambient light intensity
    waterColor = waterColor * paramAmbientLightIntensity;

    // Calculate position
    gl_Position = paramOrthoMatrix * vec4(inWaterAttribute.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec4 waterColor;

void main()
{
    gl_FragColor = waterColor;
} 
