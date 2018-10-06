###VERTEX

// Inputs
attribute vec2 inputPos;
attribute float inputTextureY;

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
varying vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
    texturePos = vec2(inputPos.x, inputTextureY);
}


###FRAGMENT

// Inputs from previous shader
varying vec2 texturePos;

// The texture
uniform sampler2D inputTexture;

// Parameters        
uniform float paramAmbientLightIntensity;
uniform float paramWaterTransparency;
uniform vec2 paramTextureScaling;

void main()
{
    vec4 textureColor = texture2D(inputTexture, texturePos * paramTextureScaling);
    gl_FragColor = vec4(textureColor.xyz, 1.0 - paramWaterTransparency) * paramAmbientLightIntensity;
} 
