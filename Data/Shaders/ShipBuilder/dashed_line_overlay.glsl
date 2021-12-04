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
    float inDash = mod(pixelCoord + 1., 2.0 * DashLength); // Shift by one, so smoothstep's are centered at DashLength's
    float lineDepth = 
        smoothstep(0.0, 2.0, inDash)
        - smoothstep(DashLength, DashLength + 2.0, inDash);
    
    gl_FragColor = mix(
        vec4(.7, .7, .7, .5),
        vec4(lineColor, 1.),
        lineDepth);
} 
