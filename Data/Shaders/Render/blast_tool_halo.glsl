###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inBlastToolHalo;  // Position, HaloSpacePosition

// Outputs
out vec2 haloSpacePosition;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    haloSpacePosition = inBlastToolHalo.zw;

    gl_Position = paramOrthoMatrix * vec4(inBlastToolHalo.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 haloSpacePosition; // (x=[-1.0, 1.0], y=[-1.0, 1.0])

void main()
{
    float r = distance(haloSpacePosition, vec2(.0, .0));
    
    #define BORDER_WIDTH 0.05
    float borderDepth = 
        smoothstep(1.0 - BORDER_WIDTH - BORDER_WIDTH, 1.0 - BORDER_WIDTH, r)
        - smoothstep(1.0 - BORDER_WIDTH, 1.0, r);
    
    ///////

    gl_FragColor = vec4(vec3(1.), borderDepth);
}
