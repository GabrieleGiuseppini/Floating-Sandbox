###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inWaterAttribute;	// Position (vec2), Texture coordinate Y (float)

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inWaterAttribute.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

void main()
{
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
} 
