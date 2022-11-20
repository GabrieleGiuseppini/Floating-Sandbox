###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inMatte1; // Vertex position (ship space)
in vec4 inMatte2; // Color

// Outputs
out vec4 matteColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    matteColor = inMatte2;
    gl_Position = paramOrthoMatrix * vec4(inMatte1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec4 matteColor;

// Params
uniform float paramOpacity;

void main()
{
    gl_FragColor = vec4(matteColor.xyz, matteColor.w * paramOpacity);
} 
