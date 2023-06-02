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

// Params
uniform float paramEffectiveAmbientLightIntensity;
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inShipGenericMipMappedTexture2.xy; 
    vertexAlpha = inShipGenericMipMappedTexture3.y;
    vertexEffectiveAmbientLightIntensity = 
        (1.0 - inShipGenericMipMappedTexture3.z)
	    + inShipGenericMipMappedTexture3.z * paramEffectiveAmbientLightIntensity;

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

#include "static_parameters.glslinc"

#define in varying

// Inputs from previous shader
in float vertexWorldY;
in vec2 vertexTextureCoordinates;
in float vertexAlpha;
in float vertexEffectiveAmbientLightIntensity;

// The texture
uniform sampler2D paramGenericMipMappedTexturesAtlasTexture;

// Parameters        
uniform vec3 paramEffectiveMoonlightColor;

void main()
{
    // Get texture sample
    vec4 textureColor = texture2D(paramGenericMipMappedTexturesAtlasTexture, vertexTextureCoordinates);

    // Z trick
    if (textureColor.w < 0.2)
        discard;

    // Apply ambient light
    textureColor.xyz *= mix(
        paramEffectiveMoonlightColor * 0.7 * clamp(1.0 + vertexWorldY / SHIP_MOONLIGHT_MAX_DEPTH, 0.0, 1.0),
        vec3(1.),
        vertexEffectiveAmbientLightIntensity);

    // Combine
    gl_FragColor = vec4(
        textureColor.xyz,
        textureColor.w * vertexAlpha);
} 
