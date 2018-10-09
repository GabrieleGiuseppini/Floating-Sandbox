###VERTEX

#version 130

// Inputs
in vec2 sharedPosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(sharedPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Params
uniform vec4 paramMatteColor;

void main()
{
    gl_FragColor = paramMatteColor;
} 
