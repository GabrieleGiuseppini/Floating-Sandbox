###VERTEX

#version 130

// Inputs
in vec2 inWaterPosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inWaterPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

void main()
{
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
} 
