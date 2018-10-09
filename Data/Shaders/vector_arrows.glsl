###VERTEX

#version 130

// Inputs
in vec2 inShipVectorPosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inShipVectorPosition.xy, -1.0, 1.0);            
}


###FRAGMENT

#version 130

// Parameters        
uniform vec3 paramMatteColor;

void main()
{
    gl_FragColor = vec4(paramMatteColor.xyz, 1.0);
} 
