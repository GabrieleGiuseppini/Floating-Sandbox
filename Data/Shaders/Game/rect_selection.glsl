###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inRectSelection1; // Vertex world position (vec2), Vertex space position (0->1) (vec2)
in vec4 inRectSelection2; // Pixel size in vertex space (vec2), Border size in vertex space (vec2)
in vec4 inRectSelection3; // Color (vec3), Elapsed (float)


// Outputs
out vec2 vertexSpacePosition;
out vec2 pixelSizeInVertexSpace;
out vec2 borderSizeInVertexSpace;
out vec3 color;
out float elapsed;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexSpacePosition = inRectSelection1.zw;
    pixelSizeInVertexSpace = inRectSelection2.xy;
    borderSizeInVertexSpace = inRectSelection2.zw;
    color = inRectSelection3.xyz;
    elapsed = inRectSelection3.w;

    gl_Position = paramOrthoMatrix * vec4(inRectSelection1.xy,  -1.0, 1.0);
}

###FRAGMENT-120

#include "common.glslinc"

#define in varying

// Inputs from previous shader
in vec2 vertexSpacePosition;
in vec2 pixelSizeInVertexSpace;
in vec2 borderSizeInVertexSpace;
in vec3 color;
in float elapsed;

void main()
{
    vec2 halfBorderSizeInVertexSpace = borderSizeInVertexSpace / 2.0;
    
    #define AntiaAliasWidthPixels 2.0
    vec2 antiAliasWidthVertexSpace = pixelSizeInVertexSpace * AntiaAliasWidthPixels;
    
    // Aspect ratio
    float ar = pixelSizeInVertexSpace.x / pixelSizeInVertexSpace.y;
    
    //
    // Anti-aliasing
    //        
    
    // 1.0 @ |1.0|-b  ->  0.0 @ |1.0|
    vec2 leftRampAlpha = vec2(1.0) - linearstep(
        vec2(1.0) - antiAliasWidthVertexSpace, 
        vec2(1.0), 
        abs(vertexSpacePosition));
    float exteriorAlpha = leftRampAlpha.x * leftRampAlpha.y;
    // 0.0 @ |1.0|-B  ->  1.0 @ |1.0|-B+b
    vec2 rightRampAlpha = linearstep(
        vec2(1.0) - borderSizeInVertexSpace, 
        vec2(1.0) - borderSizeInVertexSpace + antiAliasWidthVertexSpace, 
        abs(vertexSpacePosition));
    float interiorAlpha = min(rightRampAlpha.x + rightRampAlpha.y, 1.);
    
    //
    // On-center-border lightening, pulsing with time
    //
    
    #define Speed 1.5
    #define Period 2.0
    #define DelayPeriods 2.0    
    float elapsed2 = elapsed * Speed;
    float timeDepth = 
        step(DelayPeriods * Period, elapsed2)
        * mod(elapsed2, Period) / Period;
        
    // Offset changes from a bit outside the border to a bit inside the border
    vec2 offset = 1.7 * borderSizeInVertexSpace * (0.5 - timeDepth);
    
    vec2 lightening = vec2(1.0) - linearstep( // ls is 0.0 at center of border
        vec2(0.0),
        halfBorderSizeInVertexSpace,
        abs(abs(vertexSpacePosition) + offset - (vec2(1.) - halfBorderSizeInVertexSpace)));
            
    // Don't let lightening cross ugly when combining into scalar;
    // choose x when y > x
    float xWins = step(
        (1.0 - abs(vertexSpacePosition.x)),
        (1.0 - abs(vertexSpacePosition.y)) * ar);    
    float lighteningScalar = 
        xWins * lightening.x
        + (1. - xWins) * lightening.y;
        
    // Damp down a bit
    lighteningScalar *= 0.8;

    // Focus
    lighteningScalar *= lighteningScalar;
    
    //
    // Put it all together
    //
    
    gl_FragColor = vec4(
        color + vec3(lighteningScalar),
        exteriorAlpha * interiorAlpha);
} 
