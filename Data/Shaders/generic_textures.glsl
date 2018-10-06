###VERTEX

// Inputs
attribute vec2 inputPos;
attribute vec2 inputTextureCoords;
float inputAmbientLightSensitivity;

// Outputs
varying vec2 vertexTextureCoords;
varying float vertexAmbientLightSensitivity;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoords = inputTextureCoords; 
    vertexAmbientLightSensitivity = inputAmbientLightSensitivity;
    gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
}

###FRAGMENT

// Inputs from previous shader
varying vec2 vertexTextureCoords;
varying float vertexAmbientLightSensitivity;

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
