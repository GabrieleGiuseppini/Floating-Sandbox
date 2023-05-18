###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inBlastToolHalo1;  // Position, HaloSpacePosition
in vec2 inBlastToolHalo2;  // Progress, PersonalitySeed

// Outputs
out vec2 haloSpacePosition;
out float progress;
out float personalitySeed;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    haloSpacePosition = inBlastToolHalo1.zw;
    progress = inBlastToolHalo2.x;
    personalitySeed = inBlastToolHalo2.y;

    gl_Position = paramOrthoMatrix * vec4(inBlastToolHalo1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 haloSpacePosition; // (x=[-1.0, 1.0], y=[-1.0, 1.0])
in float progress;
in float personalitySeed;

// The texture
uniform sampler2D paramNoiseTexture;

float GetNoise(float a, float b, float c) // -> (0.0, 1.0)
{
    return texture2D(paramNoiseTexture, vec2(0.015 * b + c, a)).r;
}

void main()
{
    //
    // Noise
    //
    
    #define PI 3.14159265358979323844
    
    float theta = atan(haloSpacePosition.y, haloSpacePosition.x) / (2.0 * PI);
    float noise = GetNoise(theta, progress, personalitySeed); // 0.0 -> 1.0
    
    //
    // Border
    //
    
    float r = distance(haloSpacePosition, vec2(.0, .0));
    
    #define BORDER_WIDTH 0.05
    float whiteDepth = 
        smoothstep(1.0 - BORDER_WIDTH - BORDER_WIDTH - noise, 1.0 - BORDER_WIDTH, r)
        - smoothstep(1.0 - BORDER_WIDTH, 1.0 - BORDER_WIDTH / 2., r);
    
    whiteDepth *= r * .7;
    
    ///////

    gl_FragColor = vec4(whiteDepth, whiteDepth, whiteDepth, 1.);
}
