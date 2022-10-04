###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inGenericMipMappedTextureNdc1; // NdcPosition (vec2), TextureCoordinates (vec2)
in vec3 inGenericMipMappedTextureNdc2; // PlaneId (float), Alpha (float>, AmbientLightSensitivity (float)

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;
out float vertexEffectiveAmbientLightIntensity;

// Params
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    vertexTextureCoordinates = inGenericMipMappedTextureNdc1.zw; 
    vertexAlpha = inGenericMipMappedTextureNdc2.y;
    vertexEffectiveAmbientLightIntensity = 
        (1.0 - inGenericMipMappedTextureNdc2.z)
	    + inGenericMipMappedTextureNdc2.z * paramEffectiveAmbientLightIntensity;

    gl_Position = vec4(inGenericMipMappedTextureNdc1.xy, inGenericMipMappedTextureNdc2.x, 1.0);
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

    //if (textureColor.w < 0.2)
    //    discard;

    gl_FragColor = vec4(
        textureColor.xyz * vertexEffectiveAmbientLightIntensity,
        textureColor.w * vertexAlpha);
} 
