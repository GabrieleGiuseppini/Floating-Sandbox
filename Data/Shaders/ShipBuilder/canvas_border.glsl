###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inCanvasBorder; // Vertex position (ship space)

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inCanvasBorder, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

void main()
{
    gl_FragColor = vec4(.35, .35, .35, 1.);
} 
