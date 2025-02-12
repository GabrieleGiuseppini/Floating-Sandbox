###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inVertexShaderInput0;

void main()
{
    gl_Position = vec4(inVertexShaderInput0.xy, -1.0, 1.0);
}


###FRAGMENT-120

#define in varying

void main()
{
    //gl_FragColor = vec4(0.0, 0.0, 0.5, 1.0);
    gl_FragColor = vec4(gl_FragCoord.x, gl_FragCoord.y, 0.0, 1.0);
} 
