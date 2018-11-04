###VERTEX

#version 130

// Inputs
in vec4 inGenericTexturePackedData1; // centerPosition, vertexOffset
in vec2 inGenericTextureTextureCoordinates;
in vec4 inGenericTexturePackedData2; // scale, angle, alpha, ambientLightSensitivity

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;
out float vertexAmbientLightSensitivity;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inGenericTextureTextureCoordinates; 
    vertexAlpha = inGenericTexturePackedData2.z;
    vertexAmbientLightSensitivity = inGenericTexturePackedData2.w;

    float scale = inGenericTexturePackedData2.x;
    float angle = inGenericTexturePackedData2.y;

    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));

    vec2 worldPosition = 
        inGenericTexturePackedData1.xy 
        + rotationMatrix * inGenericTexturePackedData1.zw * scale;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;
in float vertexAmbientLightSensitivity;

// The texture
uniform sampler2D inputTexture;

// Parameters        
uniform float paramAmbientLightIntensity;

void main()
{
    vec4 textureColor = texture2D(inputTexture, vertexTextureCoordinates);

    float ambientLightIntensity = 
        (1.0 - vertexAmbientLightSensitivity)
	    + vertexAmbientLightSensitivity * paramAmbientLightIntensity;

    gl_FragColor = vec4(
        textureColor.xyz * ambientLightIntensity,
        textureColor.w * vertexAlpha);
} 
