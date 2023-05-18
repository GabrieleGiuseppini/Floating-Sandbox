###VERTEX-120

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

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 spraySpacePosition; // (x=[-0.5, 0.5], y=[-0.5, 0.5])

// The texture
uniform sampler2D paramNoiseTexture;

// Params
uniform float paramTime;

#define PI 3.14159265358979323844

mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);

    return m;
}

float GetNoise(vec2 uv) // -> (0.0, 1.0)
{
    return texture2D(paramNoiseTexture, uv).r;
}

vec2 GetSprayRadii(vec2 uv)
{      
    #define SpraySpeed 0.3
    float sprayTime = paramTime * SpraySpeed;    
    
    //
    // Rotate based on noise sampled via polar coordinates of pixel
    //
    
    // (r, a) (r=[0.0, 1.0], a=[0.0, 1.0 CCW from W])
    vec2 angleNoiseRa = vec2(
        length(uv) / sqrt(0.5), 
        (atan(uv.y, uv.x) / (2.0 * PI) + 0.5));
    
    #define AngleNoiseResolution 1.0
    float angle = GetNoise(
        angleNoiseRa * vec2(AngleNoiseResolution, AngleNoiseResolution) 
        + vec2(-0.5 * sprayTime, 0.2* sprayTime));
        
    // Magnify rotation amount based on distance from center of screen
    angle *= 1.54 * smoothstep(0.0, 0.6, angleNoiseRa.x);
    
    // Rotate!
    uv += GetRotationMatrix(angle) * uv;
    
    
    
    //
    // Transform to polar coordinates
    //
    
    // (r, a) (r=[0.0, 1.0], a=[0.0, 1.0 CCW from W])
        
    vec2 ra = vec2(
        length(uv) / sqrt(0.5), 
        (atan(uv.y, uv.x) / (2.0 * PI) + 0.5));
        
    // Scale radius to better fit in quad
    ra.x *= 1.7;                   

    
    //
    // Randomize radius based on noise and radius
    //
    
    #define VariationRNoiseResolution 1.0
    float variationR = GetNoise(
        ra * vec2(VariationRNoiseResolution / 4.0, VariationRNoiseResolution / 1.0) 
        + vec2(-sprayTime, 0.0));
    
    variationR -= 0.5;

    // Straighten the spray at the center and make full turbulence outside,
    // scaling it at the same time
    variationR *= 0.35 * smoothstep(-0.40, 0.4, ra.x);

    return vec2(ra.x + variationR, ra.x);
}

void main()
{         
    vec2 radii = GetSprayRadii(spraySpacePosition); // Randomized, Non-randomized
    
    // Focus (compress dynamics)
    float alpha = 1.0 - smoothstep(0.2, 1.4, radii.x);

    gl_FragColor = vec4(alpha);
}
