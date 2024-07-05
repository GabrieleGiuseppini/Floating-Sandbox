###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inLineGuide1; // Vertex position (NDC), Pixel coord (pixel space)

// Outputs
out float pixelCoord;

void main()
{
    pixelCoord = inLineGuide1.z;
    gl_Position = vec4(inLineGuide1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float pixelCoord;

void main()
{
    #define PIXEL_STEP 8.0

    float inDash = mod(pixelCoord, 2.0 * PIXEL_STEP);
    float lineDepth = 
        step(0.0, inDash)
        - step(PIXEL_STEP, inDash);
    
    gl_FragColor = mix(
        vec4(.7, .7, .7, .5),
        vec4(0., 0., 0., 1.),
        lineDepth);
} 
