###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inFireExtinguisherSpray; // Position, SpraySpacePosition

// Outputs
out vec2 spraySpacePosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    spraySpacePosition = inFireExtinguisherSpray.zw;

    gl_Position = paramOrthoMatrix * vec4(inFireExtinguisherSpray.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 spraySpacePosition; // (x=[-0.5, 0.5], y=[-0.5, 0.5])

// The texture
uniform sampler2D paramNoiseTexture2;

// Params
uniform float paramTime;

#define PI 3.14159265358979323844

float GetNoise(vec2 uv) // -> (0.25, 0.75)
{
    float n = (texture2D(paramNoiseTexture2, uv).r - 0.5) * 0.5;
    n += (texture2D(paramNoiseTexture2, uv * 2.0).r - 0.5) * 0.5 * 0.5;
    
    return n + 0.5;
}

float GetSprayRadius()
{      
    //
    // Transform to polar coordinates
    //
    
    // (r, a) (r=[0.0, 1.0], a=[0.0, 1.0 CCW from W])
    vec2 uv = vec2(
        length(spraySpacePosition) / sqrt(0.5), 
        (atan(spraySpacePosition.y, spraySpacePosition.x) / (2.0 * PI) + 0.5));
    
    // Scale radius to better fit in quad
    uv.x *= 1.0;
    
    
    //
    // Spray time
    //
    
    #define SpraySpeed 0.3
    float sprayTime = paramTime * SpraySpeed;
    
    
    //
    // Get noise for this fragment (in polar coordinates) and time
    //
    
    #define NoiseResolution 1.0
    float fragmentNoise = GetNoise(uv * vec2(NoiseResolution / 4.0, NoiseResolution) + vec2(-sprayTime, 0.0));
        
    
    //
    // Randomize radius based on noise and radius
    //
    
    float variationR = (fragmentNoise - 0.5);

    // Straighten the spray at the center and make full turbulence outside
    variationR *= smoothstep(-0.05, 0.4, uv.x);
    
    // Scale variation
    variationR *= 0.65;
    
    // Randomize!
    uv.x += variationR;

    return uv.x;
}

void main()
{         
    float radius1 = GetSprayRadius();
    
    // Focus (compress dynamics)
    float radius2 = smoothstep(0.2, 0.35, radius1);
    
    vec3 col1 = mix(vec3(1.0, 1.0, 1.0), vec3(0.6, 1.0, 1.0), smoothstep(0.1, 0.2, radius1));
    col1 = mix(col1, vec3(0.1, 0.4, 1.0), smoothstep(0.18, 0.25, radius1));

    gl_FragColor = vec4(col1.xyz, (1.0 - radius2) * 2.0 / 3.0);
}
