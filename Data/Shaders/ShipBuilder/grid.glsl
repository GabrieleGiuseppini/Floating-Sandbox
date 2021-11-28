###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inGrid1; // Vertex position (ship space), vertex position (physical display pixel space)
in float inGrid2; // Mid X in vertex position space above (physical display pixel space)

// Outputs
out vec2 vertexPixelSpaceCoords;
out float vertexPixelSpaceMidX;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexPixelSpaceCoords = inGrid1.zw;
    vertexPixelSpaceMidX = inGrid2;
    gl_Position = paramOrthoMatrix * vec4(inGrid1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexPixelSpaceCoords;
in float vertexPixelSpaceMidX;

// Parameters
uniform float paramPixelStep; // # of pixels

void main()
{
    //
    // Main grid
    //
    
    // 0.0 at edge of grid, 1.0 at other side
    vec2 mainGridUnary = fract(vertexPixelSpaceCoords / paramPixelStep);
    
    // 1. on grid, 0. otherwise
    float mainGridTolerance = .51 / paramPixelStep;
    float mainGridDepth = 1.0 -
        step(mainGridTolerance, mainGridUnary.x)
        * step(mainGridTolerance, mainGridUnary.y);
    
    //
    // Sub grid
    //
    
    // # of pixels
    #define SubGridStep 4.
    
    // 0.0 at edge of chop, 1.0 at other side
    // Note how we shift pixels to ensure we're smack on the main grid
    vec2 subGridUnary = fract((vertexPixelSpaceCoords + vec2(SubGridStep / 2.)) / SubGridStep);
    
    // 1. on half of the grid 0. otherwise
    float subGridTolerance = .51;
    float subGridDepth = 
        step(subGridTolerance, subGridUnary.x)
        * step(subGridTolerance, subGridUnary.y);

    //
    // Vertical guide
    //

    float verticalGuideDepth = 
        step(0.0, vertexPixelSpaceCoords.x - vertexPixelSpaceMidX)
        - step(1.1, vertexPixelSpaceCoords.x - vertexPixelSpaceMidX);
                    
    gl_FragColor = vec4(.7, .7, .7, 
        min(mainGridDepth * subGridDepth + verticalGuideDepth, 1.));
} 
