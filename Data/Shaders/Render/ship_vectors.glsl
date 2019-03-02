###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inSharedAttribute0;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inSharedAttribute0.xyz, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Params
uniform vec4 paramMatteColor;

void main()
{
    gl_FragColor = paramMatteColor;
} 
