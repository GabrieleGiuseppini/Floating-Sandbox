###VERTEX

// Inputs
attribute vec2 inputPos;

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
varying vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
    texturePos = inputPos.xy;
}

###FRAGMENT

// Inputs from previous shader
varying vec2 texturePos;

// The texture
uniform sampler2D inputTexture;

// Parameters        
uniform float paramAmbientLightIntensity;
uniform vec2 paramTextureScaling;

void main()
{
    gl_FragColor = texture2D(inputTexture, texturePos * paramTextureScaling) * paramAmbientLightIntensity;
} 
