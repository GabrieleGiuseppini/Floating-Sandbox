###VERTEX

#version 130

// Inputs
in vec2 inGenericTextureCenterPosition;
in vec2 inGenericTextureVertexOffset;
in vec2 inGenericTextureTextureCoordinates;
in float inGenericTextureRotationAngle;
in float inGenericTextureScale;
in float inGenericTextureTransparency;
in float inGenericTextureAmbientLightSensitivity;

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexTransparency;
out float vertexAmbientLightSensitivity;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inGenericTextureTextureCoordinates; 
    vertexTransparency = inGenericTextureTransparency;
    vertexAmbientLightSensitivity = inGenericTextureAmbientLightSensitivity;

    mat2 rotationMatrix = mat2(
	cos(inGenericTextureRotationAngle), -sin(inGenericTextureRotationAngle),
	sin(inGenericTextureRotationAngle), cos(inGenericTextureRotationAngle));

    vec2 position = 
	inGenericTextureCenterPosition 
	+ rotationMatrix * inGenericTextureVertexOffset * inGenericTextureScale;

    gl_Position = paramOrthoMatrix * vec4(position.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexTransparency;
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
	textureColor.w * vertexTransparency);
} 
