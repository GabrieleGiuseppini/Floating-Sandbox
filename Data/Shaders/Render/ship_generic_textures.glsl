###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inGenericTexture1; // CenterPosition, VertexOffset
in vec4 inGenericTexture2; // TextureCoordinates, PlaneId, Scale
in vec3 inGenericTexture3; // Angle, Alpha, AmbientLightSensitivity

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;
out float vertexAmbientLightIntensity;

// Params
uniform float paramAmbientLightIntensity;
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inGenericTexture2.xy; 
    vertexAlpha = inGenericTexture3.y;
    vertexAmbientLightIntensity = 
        (1.0 - inGenericTexture3.z)
	    + inGenericTexture3.z * paramAmbientLightIntensity;

    float scale = inGenericTexture2.w;
    float angle = inGenericTexture3.x;

    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));

    vec2 worldPosition = 
        inGenericTexture1.xy 
        + rotationMatrix * inGenericTexture1.zw * scale;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, inGenericTexture2.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;
in float vertexAmbientLightIntensity;

// The texture
uniform sampler2D paramGenericTexturesAtlasTexture;

// Parameters        


void main()
{
    vec4 textureColor = texture2D(paramGenericTexturesAtlasTexture, vertexTextureCoordinates);

    gl_FragColor = vec4(
        textureColor.xyz * vertexAmbientLightIntensity,
        textureColor.w * vertexAlpha);
} 
