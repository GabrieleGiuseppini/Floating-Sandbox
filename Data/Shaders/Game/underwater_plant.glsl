###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inUnderwaterPlantStatic1;  // Position, PlantSpaceCoords
in vec4 inUnderwaterPlantStatic2; // TextureInSpaceCoords, PersonalitySeed, AtlasTileIndex
in float inUnderwaterPlantDynamic1; // VertexOceanY, negative underneath

// Outputs
out vec2 vertexPlantSpaceCoords;
out vec2 vertexTextureInSpaceCoords;
out float vertexPersonalitySeed;
out float vertexAtlasTileIndex;
out float vertexOceanY;
out vec2 vertexWorld;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexPlantSpaceCoords = inUnderwaterPlantStatic1.zw;
    vertexTextureInSpaceCoords = inUnderwaterPlantStatic2.xy;
    vertexPersonalitySeed = inUnderwaterPlantStatic2.z;
    vertexAtlasTileIndex = inUnderwaterPlantStatic2.w;
    vertexOceanY = inUnderwaterPlantDynamic1;
    vertexWorld = inUnderwaterPlantStatic1.xy;

    gl_Position = paramOrthoMatrix * vec4(inUnderwaterPlantStatic1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"
#include "ocean.glslinc"

// Inputs from previous shader
in vec2 vertexPlantSpaceCoords;
in vec2 vertexTextureInSpaceCoords;
in float vertexPersonalitySeed;
in float vertexAtlasTileIndex;
in float vertexOceanY;
in vec2 vertexWorld;

// The texture
uniform vec4 paramAtlasTileGeometryIndexed[7 * 2]; // Keep size with # of underwater plant textures
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

// Params
uniform float paramSimulationTime;
uniform float paramUnderwaterPlantRotationAngle;
uniform float paramUnderwaterCurrentSpaceVelocity;
uniform float paramUnderwaterCurrentTimeVelocity;
uniform float paramWindDirection;

uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanDepthDarkeningRate;


mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);

    return m;
}

void main()
{
    int vertexAtlasTileIndexI = int(vertexAtlasTileIndex);

    // Negative underneath
    float vertexWorldOceanDepth = vertexWorld.y - vertexOceanY;

    // Underwater pulse: -1..1
    float underwaterPulse = sin(paramUnderwaterCurrentSpaceVelocity * vertexWorld.x + paramUnderwaterCurrentTimeVelocity * paramSimulationTime + vertexPersonalitySeed * PI / 3.);
    
    // Rotation angle is higher the higher we go
    float maxRotAngle = paramUnderwaterPlantRotationAngle * underwaterPulse;
    float angle = maxRotAngle * vertexPlantSpaceCoords.y;

    // X ripples
    float xRipples = 0.009 * sin(30.37 * vertexWorld.x + paramSimulationTime * 5.9) * step(vertexWorldOceanDepth, 0.0);
    angle += paramUnderwaterPlantRotationAngle * xRipples;    
    
    // Plant flattening
    angle = mix(
        angle,
        PI / 2.0 * paramWindDirection,
        smoothstep(-3.0, 0.0, vertexWorldOceanDepth)); // Still angle below surface; flat on surface
            
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
    vec4 underwaterPlantSample = texture2D(paramGenericLinearTexturesAtlasTexture, textureCoords);

    //////////////////////////////////////

    // Calculate depth darkening
    float darkeningFactor = CalculateOceanDepthDarkeningFactor(
        vertexWorld.y,
        paramOceanDepthDarkeningRate);

    // Calculate lamp tool intensity
    float lampToolIntensity = CalculateLampToolIntensity(gl_FragCoord.xy);

    // Apply depth-darkening
    underwaterPlantSample.xyz = mix(
        underwaterPlantSample.xyz,
        vec3(0.),
        darkeningFactor * (1.0 - lampToolIntensity));

    // Apply ambient light
    underwaterPlantSample.xyz *= max(paramEffectiveAmbientLightIntensity, lampToolIntensity);

    gl_FragColor = underwaterPlantSample;
}
