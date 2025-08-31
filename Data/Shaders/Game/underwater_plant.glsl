###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inUnderwaterPlantStatic1;  // Position, PlantSpaceCoords
in vec3 inUnderwaterPlantStatic2; // TextureWidthInSpaceCoords, PersonalitySeed, SpeciesIndex
in float inUnderwaterPlantDynamic1; // VertexWorldOceanRelativeY, negative underneath

// Outputs
out vec2 vertexPlantSpaceCoords;
out float vertexTextureWidthInSpaceCoords;
out float vertexPersonalitySeed;
out float vertexSpeciesIndex;
out float vertexWorldOceanRelativeY;
out float vertexWorldX;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexPlantSpaceCoords = inUnderwaterPlantStatic1.zw;
    vertexTextureWidthInSpaceCoords = inUnderwaterPlantStatic2.x;
    vertexPersonalitySeed = inUnderwaterPlantStatic2.y;
    vertexSpeciesIndex = inUnderwaterPlantStatic2.z;
    vertexWorldOceanRelativeY = inUnderwaterPlantDynamic1;
    vertexWorldX = inUnderwaterPlantStatic1.x;

    gl_Position = paramOrthoMatrix * vec4(inUnderwaterPlantStatic1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexPlantSpaceCoords;
in float vertexTextureWidthInSpaceCoords;
in float vertexPersonalitySeed;
in float vertexSpeciesIndex;
in float vertexWorldOceanRelativeY;
in float vertexWorldX;

// The texture
uniform vec4 paramAtlasTileGeometryIndexed[4]; // Keep size with # of underwater plant textures
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

    int vertexSpeciesIndexI = int(vertexSpeciesIndex);

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
    vec2 virtualTextureCoords = vec2(
        (rotatedPlantSpacePosition.x / vertexTextureWidthInSpaceCoords) + 0.5,
        rotatedPlantSpacePosition.y);
    virtualTextureCoords = clamp(virtualTextureCoords, vec2(0.), vec2(1.0));    
    
    // Map virtual texture coords to atlas coords
    vec2 textureCoords = 
        paramAtlasTileGeometryIndexed[vertexSpeciesIndexI].xy
        + paramAtlasTileGeometryIndexed[vertexSpeciesIndexI].zw * virtualTextureCoords;
    
    // Sample!
    gl_FragColor = texture2D(paramGenericLinearTexturesAtlasTexture, textureCoords);

    // TODOTEST

    //vec2 textureCoords = paramAtlasTileGeometryIndexed[vertexSpeciesIndexI].xy + vertexTextureCoords * paramAtlasTileGeometryIndexed[vertexSpeciesIndexI].zw;
    //gl_FragColor = texture2D(paramGenericLinearTexturesAtlasTexture, textureCoords);
}
