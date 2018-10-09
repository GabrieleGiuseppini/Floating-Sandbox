###VERTEX

#version 130

// Inputs
in vec2 genericTexturePosition;
in vec2 genericTextureCoordinates;
in float genericTextureAmbientLightSensitivity;

// Outputs
out vec2 vertexTextureCoords;
out float vertexAmbientLightSensitivity;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoords = genericTextureCoordinates; 
    vertexAmbientLightSensitivity = genericTextureAmbientLightSensitivity;
    gl_Position = paramOrthoMatrix * vec4(genericTexturePosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 vertexTextureCoords;
in float vertexAmbientLightSensitivity;

// The texture
uniform sampler2D inputTexture;

// Parameters        
uniform float paramAmbientLightIntensity;

void main()
{
    vec4 textureColor = texture2D(inputTexture, vertexTextureCoords);

    float ambientLightIntensity = 
	(1.0 - vertexAmbientLightSensitivity)
	+ vertexAmbientLightSensitivity * paramAmbientLightIntensity;

    gl_FragColor = vec4(
	textureColor.xyz * ambientLightIntensity,
	textureColor.w);
} 
