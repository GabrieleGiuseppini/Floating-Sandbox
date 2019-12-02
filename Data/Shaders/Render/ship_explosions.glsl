###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inExplosion1; // CenterPosition, VertexOffset
in vec4 inExplosion2; // TextureCoordinates, PlaneId, Angle
in float inExplosion3; // Progress

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexProgress;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inExplosion2.xy; 
    vertexProgress = inExplosion3;

    float angle = inExplosion2.w;

    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));

    vec2 worldPosition = 
        inExplosion1.xy 
        + rotationMatrix * inExplosion1.zw;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, inExplosion2.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates; // (0.0, 0.0) (bottom-left) -> (1.0, 1.0)
in float vertexProgress;

// The texture
uniform sampler2D paramExplosionsAtlasTexture;

// Parameters

// Actual frames, there is also an implicit pre-frame and a post-frame
#define NTextureFrames 16.

// Square root of number of frames
#define AtlasSideSize 4.

// Size of a frame side, in texture coords
#define FrameSizeWidth 1. / AtlasSideSize

vec4 SampleColor(float frameIndex, vec2 uv)
{   
    float c = mod(frameIndex, AtlasSideSize);
    float r = floor(frameIndex / AtlasSideSize);
    
    // Transform uv into coords of frame in Atlas
    vec2 atlasSampleCoords = (vec2(c, r) + uv) * FrameSizeWidth;
    
    // Clamp
    atlasSampleCoords = vec2(atlasSampleCoords.x, clamp(0., 1., atlasSampleCoords.y));
    
    // Sample
    return texture2D(paramExplosionsAtlasTexture, atlasSampleCoords);
}

void main()
{
    float sigma = 1./(NTextureFrames + 1.);
    float bucket = floor(vertexProgress / sigma);
    float inBucket = fract(vertexProgress / sigma);
    
    //
    // At any given moment in time we draw two frames:
    // - Frame 1: index=(bucket - 1.), alpha=(1. - inBucket)
    // - Frame 2: index=(bucket), alpha=(inBucket)    
    //
        
    
    //
    // Scale
    //
    
    #define ScaleDelta .1
    float scale1 = 1. + ScaleDelta * inBucket;
    float scale2 = scale1 - ScaleDelta;       
    
    
    //
    // Sample
    //
    
    vec2 centeredSpacePosition = vertexTextureCoordinates - vec2(.5);
    vec4 c1 = SampleColor(bucket - 1., centeredSpacePosition / scale1 + vec2(.5));
    vec4 c2 = SampleColor(bucket, centeredSpacePosition / scale2 + vec2(.5));
    gl_FragColor = mix(c1, c2, inBucket);
} 
