###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inCenter1; // Position, SpacePosition
in float inCenter2; // PlaneId

// Outputs        
out vec2 vertexSpacePosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexSpacePosition = inCenter1.zw;
    gl_Position = paramOrthoMatrix * vec4(inCenter1.xy, inCenter2, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader        
in vec2 vertexSpacePosition;

void main()
{
    #define CircleWidth 0.15
    
    float radius = length(vertexSpacePosition);
    
    // Circles
    float depth =
        // External circle
        smoothstep(1.0 - CircleWidth * 2., 1.0 - CircleWidth, radius)
        - smoothstep(1.0 - CircleWidth, 1.0, radius)
        // Internal circle
        + smoothstep(.5 - CircleWidth * 2., .5 - CircleWidth, radius)
        - smoothstep(.5 - CircleWidth, .5, radius);
        
    // Arms
    depth += 
        (1. - smoothstep(0., CircleWidth / 4., abs(vertexSpacePosition.x) * abs(vertexSpacePosition.y)))
        * step(radius, 1. - CircleWidth)
        * step(.5 - CircleWidth, radius);

    if (depth < 0.2)
        discard;

    gl_FragColor = vec4(
        depth, 
        0., 
        0., 
        depth);
} 
