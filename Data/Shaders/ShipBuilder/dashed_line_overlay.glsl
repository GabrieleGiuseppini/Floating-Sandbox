###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inDashedLineOverlay1; // Vertex position (ship space), Pixel coord (pixel space)
in vec3 inDashedLineOverlay2; // Color

// Outputs
out float pixelCoord;
out vec3 lineColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    pixelCoord = inDashedLineOverlay1.z;
    lineColor = inDashedLineOverlay2;
    gl_Position = paramOrthoMatrix * vec4(inDashedLineOverlay1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float pixelCoord;
in vec3 lineColor;

void main()
{
    #define DashLength 4.0
    float lineDepth = step(mod(pixelCoord, 2.0 * DashLength), DashLength);
    
    gl_FragColor = vec4(lineColor, lineDepth);
} 
