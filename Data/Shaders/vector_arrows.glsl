###VERTEX

// Inputs
attribute vec2 inputPos;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);            
}


###FRAGMENT

// Parameters        
uniform vec3 paramMatteColor;

void main()
{
    gl_FragColor = vec4(paramMatteColor.xyz, 1.0);
} 
