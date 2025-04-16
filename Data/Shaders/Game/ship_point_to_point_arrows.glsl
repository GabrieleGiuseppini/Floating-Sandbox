###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inPointToPointArrow1; // Position, PlaneId
in vec3 inPointToPointArrow2; // Color

// Outputs        
out vec3 vertexColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexColor = inPointToPointArrow2;
    gl_Position = paramOrthoMatrix * vec4(inPointToPointArrow1.xyz, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader        
in vec3 vertexColor;

void main()
{
    gl_FragColor = vec4(vertexColor, 1.0);
} 
