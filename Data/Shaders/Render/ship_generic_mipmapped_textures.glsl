###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inGenericMipMappedTexture1; // CenterPosition, VertexOffset
in vec4 inGenericMipMappedTexture2; // TextureCoordinates, PlaneId, Scale
in vec3 inGenericMipMappedTexture3; // Angle, Alpha, AmbientLightSensitivity

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;
out float vertexEffectiveAmbientLightIntensity;

// Params
uniform float paramEffectiveAmbientLightIntensity;
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inGenericMipMappedTexture2.xy; 
    vertexAlpha = inGenericMipMappedTexture3.y;
    vertexEffectiveAmbientLightIntensity = 
        (1.0 - inGenericMipMappedTexture3.z)
	    + inGenericMipMappedTexture3.z * paramEffectiveAmbientLightIntensity;

    float scale = inGenericMipMappedTexture2.w;
    float angle = inGenericMipMappedTexture3.x;

    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));

    vec2 worldPosition = 
        inGenericMipMappedTexture1.xy 
        + rotationMatrix * inGenericMipMappedTexture1.zw * scale;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, inGenericMipMappedTexture2.z, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;
in float vertexEffectiveAmbientLightIntensity;

// The texture
uniform sampler2D paramGenericMipMappedTexturesAtlasTexture;

// Parameters        


void main()
{
    vec4 textureColor = texture2D(paramGenericMipMappedTexturesAtlasTexture, vertexTextureCoordinates);

    if (textureColor.w < 0.2)
        discard;

    gl_FragColor = vec4(
        textureColor.xyz * vertexEffectiveAmbientLightIntensity,
        textureColor.w * vertexAlpha);
} 
