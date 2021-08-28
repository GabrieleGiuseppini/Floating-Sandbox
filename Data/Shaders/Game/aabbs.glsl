###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inAABB1; // Color (vec4)
in vec2 inAABB2; // Position (vec2)

// Outputs
out vec4 vertexColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexColor = inAABB1;

    gl_Position = paramOrthoMatrix * vec4(inAABB2, -1.0, 1.0);   
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec4 vertexColor;

void main()
{
    gl_FragColor = vertexColor;
} 
