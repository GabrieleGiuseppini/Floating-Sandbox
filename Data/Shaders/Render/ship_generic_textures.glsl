###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inGenericTexturePackedData1; // centerPosition, vertexOffset
in vec4 inGenericTexturePackedData2; // textureCoordinates, planeId, scale
in vec3 inGenericTexturePackedData3; // angle, alpha, ambientLightSensitivity

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;
out float vertexAmbientLightIntensity;

// Params
uniform float paramAmbientLightIntensity;
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inGenericTexturePackedData2.xy; 
    vertexAlpha = inGenericTexturePackedData3.y;
    vertexAmbientLightIntensity = 
        (1.0 - inGenericTexturePackedData3.z)
	    + inGenericTexturePackedData3.z * paramAmbientLightIntensity;

    float scale = inGenericTexturePackedData2.w;
    float angle = inGenericTexturePackedData3.x;

    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));

    vec2 worldPosition = 
        inGenericTexturePackedData1.xy 
        + rotationMatrix * inGenericTexturePackedData1.zw * scale;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, inGenericTexturePackedData2.z, 1.0);
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
