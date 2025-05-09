###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inShipGenericMipMappedTexture1; // CenterPosition, VertexOffset
in vec4 inShipGenericMipMappedTexture2; // TextureCoordinates, PlaneId, Scale
in vec3 inShipGenericMipMappedTexture3; // Angle, Alpha, AmbientLightSensitivity

// Outputs
out float vertexWorldY;
out vec2 vertexTextureCoordinates;
out float vertexAlpha;
out float vertexEffectiveAmbientLightIntensity;
out float vertexEffectiveDepthDarkeningSensitivity;

// Params
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramShipDepthDarkeningSensitivity;
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inShipGenericMipMappedTexture2.xy; 
    vertexAlpha = inShipGenericMipMappedTexture3.y;
    vertexEffectiveAmbientLightIntensity = 
        (1.0 - inShipGenericMipMappedTexture3.z)
	    + inShipGenericMipMappedTexture3.z * paramEffectiveAmbientLightIntensity;
    vertexEffectiveDepthDarkeningSensitivity =
	    inShipGenericMipMappedTexture3.z * paramShipDepthDarkeningSensitivity;

    float scale = inShipGenericMipMappedTexture2.w;
    float angle = inShipGenericMipMappedTexture3.x;

    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));

    vec2 worldPosition = 
        inShipGenericMipMappedTexture1.xy 
        + rotationMatrix * inShipGenericMipMappedTexture1.zw * scale;

    vertexWorldY = worldPosition.y;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, inShipGenericMipMappedTexture2.z, 1.0);
}

###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"
#include "static_parameters.glslinc"

// Inputs from previous shader
in float vertexWorldY;
in vec2 vertexTextureCoordinates;
in float vertexAlpha;
in float vertexEffectiveAmbientLightIntensity;
in float vertexEffectiveDepthDarkeningSensitivity;

// The texture
uniform sampler2D paramGenericMipMappedTexturesAtlasTexture;

// Parameters        
uniform vec3 paramEffectiveMoonlightColor;
uniform float paramOceanDepthDarkeningRate;


void main()
{
    // Get texture sample
    vec4 textureColor = texture2D(paramGenericMipMappedTexturesAtlasTexture, vertexTextureCoordinates);

    // Z trick
    if (textureColor.w < 0.2)
        discard;

    // Calculate lamp tool intensity
    float lampToolIntensity = CalculateLampToolIntensity(gl_FragCoord.xy);

    if (vertexEffectiveDepthDarkeningSensitivity > 0.0) // Fine to branch - all pixels will follow the same branching
    {
        // Calculate depth darkening
        float darkeningFactor = CalculateOceanDepthDarkeningFactor(
            vertexWorldY,
            paramOceanDepthDarkeningRate);

        // Apply depth darkening
        textureColor.xyz = mix(
            textureColor.xyz,
            vec3(0.),
            darkeningFactor * (1.0 - lampToolIntensity) * vertexEffectiveDepthDarkeningSensitivity);
    }

    // Apply ambient light
    textureColor.xyz = ApplyAmbientLight(
        textureColor.xyz,
        paramEffectiveMoonlightColor * 0.7 * clamp(1.0 + vertexWorldY / SHIP_MOONLIGHT_MAX_DEPTH, 0.0, 1.0),
        vertexEffectiveAmbientLightIntensity,
        lampToolIntensity);
    
    // Combine
    gl_FragColor = vec4(
        textureColor.xyz,
        textureColor.w * vertexAlpha);
} 
