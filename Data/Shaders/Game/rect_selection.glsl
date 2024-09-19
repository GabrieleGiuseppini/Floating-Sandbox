###VERTEX-120

#define in attribute
#define out varying

// Inputs
// TODOHERE
in vec4 inRectSelection1; // Vertex position (NDC), Pixel coord (pixel space)

// Outputs
//out float pixelCoord;

void main()
{
    //pixelCoord = inLineGuide1.z;
    gl_Position = vec4(inRectSelection1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
//in float pixelCoord;

void main()
{
    gl_FragColor = vec4(.7, .7, .7, .5);
} 
