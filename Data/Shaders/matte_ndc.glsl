###VERTEX

#version 130

// Inputs
in vec2 inputPos;

void main()
{
    gl_Position = vec4(inputPos.xy, -1.0, 1.0);
}


###FRAGMENT

#version 130

// Params
uniform vec4 paramMatteColor;

void main()
{
    gl_FragColor = paramMatteColor;
} 
