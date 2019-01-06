###VERTEX

#version 130

// Inputs
in vec2 inVertexCoords;

void main()
{
    gl_Position = vec4(inVertexCoords.xy, -1.0, 1.0);
}


###FRAGMENT

#version 130

// Params
//uniform vec4 paramMatteColor;

void main()
{
    //gl_FragColor = vec4(0.0, 0.0, 0.5, 1.0);
    gl_FragColor = vec4(gl_FragCoord.x, gl_FragCoord.y, 0.0, 1.0);
} 
