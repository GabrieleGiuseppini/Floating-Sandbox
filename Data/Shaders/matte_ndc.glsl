###VERTEX

// Inputs
attribute vec2 inputPos;

void main()
{
    gl_Position = vec4(inputPos.xy, -1.0, 1.0);
}


###FRAGMENT

// Params
uniform vec4 paramMatteColor;

void main()
{
    gl_FragColor = paramMatteColor;
} 
