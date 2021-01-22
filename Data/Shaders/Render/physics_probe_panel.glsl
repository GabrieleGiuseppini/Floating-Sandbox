###VERTEX-120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inPhysicsProbePanel1; // Position (vec2), TextureCoordinates (vec2)
in vec2 inPhysicsProbePanel2; // XLimits (vec2)

// Outputs
out vec2 vertexCoordinatesNdc;
out vec2 vertexTextureCoordinates;
out vec2 xLimitsNdc;

void main()
{
    vertexCoordinatesNdc = inPhysicsProbePanel1.xy;
    vertexTextureCoordinates = inPhysicsProbePanel1.zw; 
    xLimitsNdc = inPhysicsProbePanel2;
    gl_Position = vec4(inPhysicsProbePanel1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexCoordinatesNdc;
in vec2 vertexTextureCoordinates;
in vec2 xLimitsNdc;

// Textures
uniform sampler2D paramGenericLinearTexturesAtlasTexture;
uniform sampler2D paramNoiseTexture2;

// Params
uniform float paramWidthNdc;

float GetNoise(float y, float seed, float time) // -> (0.0, 1.0)
{
    #define NOISE_RESOLUTION .5
    float s1 = texture2D(paramNoiseTexture2, vec2(NOISE_RESOLUTION * y + time, seed)).r;
    float s2 = texture2D(paramNoiseTexture2, vec2(NOISE_RESOLUTION * y - time, seed)).r;
    
    #define NOISE_THRESHOLD .65
    s1 = (clamp(s1, NOISE_THRESHOLD, 1.) - NOISE_THRESHOLD) * 1. / (1. - NOISE_THRESHOLD);
    s2 = (clamp(s2, NOISE_THRESHOLD, 1.) - NOISE_THRESHOLD) * 1. / (1. - NOISE_THRESHOLD);
    
    return (s1 + s2) / 2.;
}

void main()
{
    //
    // Frontier
    //
    
    float currentWidth = xLimitsNdc.y - xLimitsNdc.x; // 0.0 -> 2.0
    float midXNdc = xLimitsNdc.x + currentWidth / 2.;
    float widthFraction = currentWidth / paramWidthNdc;
    
    float noise = GetNoise(vertexCoordinatesNdc.y / paramWidthNdc * 2., .2 * step(midXNdc, vertexCoordinatesNdc.x), widthFraction / 8.);
    
    float flangeLength = (0.05 + noise * .4) * paramWidthNdc;
    
    // Flatten flange when too small or almost fully open
    flangeLength *= (smoothstep(.0, .46, widthFraction) - smoothstep(.85, 1., widthFraction));
    
    float frontierDepth = min(
        smoothstep(0.0, flangeLength, vertexCoordinatesNdc.x - xLimitsNdc.x),
        smoothstep(0.0, flangeLength, xLimitsNdc.y - vertexCoordinatesNdc.x));
                
        
    //
    // Texture
    //
    
    vec4 cTexture = texture2D(paramGenericLinearTexturesAtlasTexture, vertexTextureCoordinates);    
           
           
    //
    // Final color
    //

    gl_FragColor = mix(
        cTexture,
        vec4(1., 1., 1., step(xLimitsNdc.x, vertexCoordinatesNdc.x) * step(vertexCoordinatesNdc.x, xLimitsNdc.y)),
        1. - frontierDepth);
} 
