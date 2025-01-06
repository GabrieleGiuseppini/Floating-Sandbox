
// Kept in sync with code
#define EXPLOSION_TYPE_DEFAULT 1.0
#define EXPLOSION_TYPE_FIRE_EXTINGUISHING 2.0
#define EXPLOSION_TYPE_SODIUM 3.0

###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inExplosion1; // CenterPosition, VertexOffset
in vec4 inExplosion2; // TextureCoordinates, PlaneId, Angle
in vec3 inExplosion3; // ExplosionIndex, ExplosionType, Progress

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexExplosionIndex;
out float vertexExplosionType;
out float vertexProgress;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inExplosion2.xy; 
    vertexExplosionIndex = inExplosion3.x;
    vertexExplosionType = inExplosion3.y;
    vertexProgress = inExplosion3.z;

    float angle = inExplosion2.w;

    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));

    vec2 worldPosition = 
        inExplosion1.xy 
        + rotationMatrix * inExplosion1.zw;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, inExplosion2.z, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates; // (0.0, 0.0) (bottom-left) -> (1.0, 1.0)
in float vertexExplosionIndex;
in float vertexExplosionType;
in float vertexProgress; // 0.0 -> 1.0

// The texture
uniform sampler2D paramExplosionsAtlasTexture;

// Constants

// Actual frames per explosion, there is also an implicit pre-frame and a post-frame
#define NExplosionFrames 16.

// Number of frames per atlas side
#define AtlasSideFrames 8.

// Size of a frame side, in texture coords
#define FrameSideSize 1. / AtlasSideFrames

float is_type(float notification_type, float value)
{
    return step(value - 0.5, notification_type) * step(notification_type, value + 0.5);
}

vec4 sample_texture(float frameIndex, vec2 uv)
{   
    // Row at which the desired explosion starts
    float explosionIndexRowStart = 2. * vertexExplosionIndex;

    float c = mod(frameIndex, AtlasSideFrames);
    float r = floor(frameIndex / AtlasSideFrames) + explosionIndexRowStart;

    // Coords on frame grid having integral indices,
    // clamped to the two rows of this explosion index
    vec2 atlasCoords = vec2(
        c + uv.x, 
        clamp(
            r + uv.y,
            explosionIndexRowStart, // Assuming here it's alpha==0 for every x
            explosionIndexRowStart + 2.));
    
    // Transform into texture coords
    vec2 atlasSampleCoords = atlasCoords * FrameSideSize;
    
    // Sample
    return texture2D(paramExplosionsAtlasTexture, atlasSampleCoords);
}

void main()
{
    //
    // At any given moment in time we draw two frames:
    // - Frame 1: index=(bucket - 1.), alpha=(1. - inBucket) (1.0 -> 0.0), scale=(1. + Delta * inBucket) (1.0 -> 1.0+Delta)
    // - Frame 2: index=(bucket), alpha=(inBucket) (0.0 -> 1.0), scale= (1. + Delta * inBucket - Delta) (1.0-Delta -> 1.0)
    //
    
    float sigma = 1./(NExplosionFrames + 1.);
    float bucket = floor(vertexProgress / sigma);
    float inBucket = fract(vertexProgress / sigma);
            
    
    //
    // Scale
    //
    
    #define ScaleDelta .16
    float scale1 = 1. + ScaleDelta * inBucket;
    float scale2 = scale1 - ScaleDelta;       
    
    
    //
    // Sample
    //
    
    vec2 centeredSpacePosition = vertexTextureCoordinates - vec2(.5);
    vec4 c1 = sample_texture(bucket - 1., centeredSpacePosition / scale1 + vec2(.5));
    vec4 c2 = sample_texture(bucket, centeredSpacePosition / scale2 + vec2(.5));

    vec4 c = mix(c1, c2, inBucket);


    //
    // Fire-Extinguishing
    //

    vec4 fire_extinguishing_color;
    {
        // TODOHERE
        fire_extinguishing_color = vec4(c.r * 0.2, c.g * 0.2, c.b, c.a);
    }


    //
    // Sodium
    //

    vec4 sodium_color;
    {
        sodium_color = vec4((c.r + c.g) / 2., (c.r + c.g) / 2., c.b, c.a);
    }


    //
    // Combine
    //

    gl_FragColor =
        c * is_type(vertexExplosionType, EXPLOSION_TYPE_DEFAULT)
        + fire_extinguishing_color * is_type(vertexExplosionType, EXPLOSION_TYPE_FIRE_EXTINGUISHING)
        + sodium_color * is_type(vertexExplosionType, EXPLOSION_TYPE_SODIUM);
} 
