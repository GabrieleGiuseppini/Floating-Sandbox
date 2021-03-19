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

// The texture
uniform sampler2D paramNoiseTexture2;

float GetNoise(float x) // -> (0.0, 1.0)
{
    return texture2D(paramNoiseTexture2, vec2(0.5, x)).r;
}

void main()
{
    //
    // Noise
    //
    
    #define PI 3.14159265358979323844
    
    float theta = atan(haloSpacePosition.y, haloSpacePosition.x) / (2.0 * PI);
    float noise = GetNoise(theta); // 0.0 -> 1.0
    
    //
    // Border
    //
    
    float r = distance(haloSpacePosition, vec2(.0, .0));
    
    #define BORDER_WIDTH 0.05
    float whiteDepth = 
        smoothstep(1.0 - BORDER_WIDTH - BORDER_WIDTH - noise, 1.0 - BORDER_WIDTH, r)
        - smoothstep(1.0 - BORDER_WIDTH, 1.0, r);

    whiteDepth *= .7;
    
    ///////

    gl_FragColor = vec4(vec3(1.), whiteDepth);
}
