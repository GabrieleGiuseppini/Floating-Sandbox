###VERTEX

#version 130

// Inputs
in vec2 inSharedPosition;

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inSharedPosition.xy, -1.0, 1.0);
    texturePos = inSharedPosition.xy;
}

###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 texturePos;

// The texture
uniform sampler2D inputTexture;

// Parameters        
uniform float paramAmbientLightIntensity;
uniform vec2 paramTextureScaling;

void main()
{
    gl_FragColor = texture2D(inputTexture, texturePos * paramTextureScaling) * paramAmbientLightIntensity;
} 
