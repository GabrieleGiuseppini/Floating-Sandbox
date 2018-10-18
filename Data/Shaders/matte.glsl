###VERTEX

#version 130

// Inputs
in vec2 inSharedAttribute0;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inSharedAttribute0.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Params
uniform vec4 paramMatteColor;

void main()
{
    gl_FragColor = paramMatteColor;
} 
