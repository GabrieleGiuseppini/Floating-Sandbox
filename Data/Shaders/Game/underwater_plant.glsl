###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inUnderwaterPlantStatic1;  // Position, PlantSpaceCoords
in vec4 inUnderwaterPlantStatic2; // TextureInSpaceCoords, PersonalitySeed, AtlasTileIndex
in float inUnderwaterPlantDynamic1; // VertexWorldOceanRelativeY, negative underneath

// Outputs
out vec2 vertexPlantSpaceCoords;
out vec2 vertexTextureInSpaceCoords;
out float vertexPersonalitySeed;
out float vertexAtlasTileIndex;
out float vertexWorldOceanRelativeY;
out float vertexWorldX;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexPlantSpaceCoords = inUnderwaterPlantStatic1.zw;
    vertexTextureInSpaceCoords = inUnderwaterPlantStatic2.xy;
    vertexPersonalitySeed = inUnderwaterPlantStatic2.z;
    vertexAtlasTileIndex = inUnderwaterPlantStatic2.w;
    //vertexWorldOceanRelativeY = inUnderwaterPlantDynamic1;
    vertexWorldOceanRelativeY = -100;
    vertexWorldX = inUnderwaterPlantStatic1.x;

    gl_Position = paramOrthoMatrix * vec4(inUnderwaterPlantStatic1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexPlantSpaceCoords;
in vec2 vertexTextureInSpaceCoords;
in float vertexPersonalitySeed;
in float vertexAtlasTileIndex;
in float vertexWorldOceanRelativeY;
in float vertexWorldX;

// The texture
uniform vec4 paramAtlasTileGeometryIndexed[4 * 2]; // Keep size with # of underwater plant textures
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

// Params
uniform float paramSimulationTime;
uniform float paramUnderwaterPlantRotationAngle;
uniform float paramUnderwaterCurrentSpaceVelocity;
uniform float paramUnderwaterCurrentTimeVelocity;

mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);

    return m;
}

void main()
{
    #define PI 3.1415926535

    int vertexAtlasTileIndexI = int(vertexAtlasTileIndex);

    // Underwater pulse: -1..1
    float underwaterPulse = sin(paramUnderwaterCurrentSpaceVelocity * vertexWorldX + paramUnderwaterCurrentTimeVelocity * paramSimulationTime + vertexPersonalitySeed * PI / 6.);
    
    // Rotation angle is higher the higher we go
    float maxRotAngle = paramUnderwaterPlantRotationAngle * underwaterPulse;
    float angle = maxRotAngle * vertexPlantSpaceCoords.y;

    // X ripples
    float xRipples = 0.011 * sin(30.37 * vertexWorldX + paramSimulationTime * 5.9) * step(vertexWorldOceanRelativeY, 0.0);
    angle += paramUnderwaterPlantRotationAngle * xRipples;    
    
    // Plant flattening (stiffening)
    angle = mix(
        0.0,
        angle,
        1.0 + clamp(-vertexWorldOceanRelativeY, -1.0, 0.0)); // still angle on surface; upright above surface
            
    // Rotate around bottom
    vec2 rotatedPlantSpacePosition = vertexPlantSpaceCoords * GetRotationMatrix(angle);
    
    //
    // Sample texture
    //
    
    // Map rotated plant-space coords to virtual texture coords (0..1)
    vec2 virtualTextureCoords = 
        rotatedPlantSpacePosition * vertexTextureInSpaceCoords 
        + vec2(0.5, 0.0); // Center x
    virtualTextureCoords = clamp(virtualTextureCoords, vec2(0.), vec2(1.0)); // Relying on transparent border in tiles
    
    // Map virtual texture coords to atlas coords
    vec2 textureCoords = mix(
            paramAtlasTileGeometryIndexed[vertexAtlasTileIndexI].xy,
            paramAtlasTileGeometryIndexed[vertexAtlasTileIndexI].zw,
            virtualTextureCoords);
    
    // Sample!
    gl_FragColor = texture2D(paramGenericLinearTexturesAtlasTexture, textureCoords);

    // TODOTEST

    //vec2 textureCoords = paramAtlasTileGeometryIndexed[vertexSpeciesIndexI].xy + vertexTextureCoords * paramAtlasTileGeometryIndexed[vertexSpeciesIndexI].zw;
    //gl_FragColor = texture2D(paramGenericLinearTexturesAtlasTexture, textureCoords);
}
