###VERTEX

#version 130

// Inputs
in vec4 inGenericTexturePackedData1; // centerPosition, vertexOffset
in vec2 inGenericTextureTextureCoordinates;
in vec4 inGenericTexturePackedData2; // rotationAngle, scale, transparency, ambientLightSensitivity

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexTransparency;
out float vertexAmbientLightSensitivity;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inGenericTextureTextureCoordinates; 
    vertexTransparency = inGenericTexturePackedData2.z;
    vertexAmbientLightSensitivity = inGenericTexturePackedData2.w;

    mat2 rotationMatrix = mat2(
	cos(inGenericTexturePackedData2.x), -sin(inGenericTexturePackedData2.x),
	sin(inGenericTexturePackedData2.x), cos(inGenericTexturePackedData2.x));

    vec2 position = 
	inGenericTexturePackedData1.xy 
	+ rotationMatrix * inGenericTexturePackedData1.zw * inGenericTexturePackedData2.y;

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
