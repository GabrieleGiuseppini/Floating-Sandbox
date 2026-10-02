###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inVectorArrow1; // Position, PlaneId
in vec3 inVectorArrow2; // Color

// Outputs
out vec3 vertexColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexColor = inVectorArrow2;
    gl_Position = paramOrthoMatrix * vec4(inVectorArrow1, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs
in vec3 vertexColor;

void main()
{
    gl_FragColor = vec4(vertexColor, 1.0);
} 
