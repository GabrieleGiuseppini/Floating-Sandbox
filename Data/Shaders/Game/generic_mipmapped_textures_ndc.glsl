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
uniform vec3 paramEffectiveMoonlightColor;

void main()
{
    vec4 textureColor = texture2D(paramGenericMipMappedTexturesAtlasTexture, vertexTextureCoordinates);

    // Apply ambient light
    textureColor.xyz *= mix(
        paramEffectiveMoonlightColor,
        vec3(1.),
        vertexEffectiveAmbientLightIntensity);

    // Combine
    gl_FragColor = vec4(
        textureColor.xyz,
        textureColor.w * vertexAlpha);
} 
