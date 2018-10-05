###VERTEX

// Inputs
attribute vec2 inputPos;
attribute vec2 inputTexturePos;

// Outputs
varying vec2 texturePos;

void main()
{
    gl_Position = vec4(inputPos.xy, %ZDEPTH%, 1.0);
    texturePos = inputTexturePos;
}

###FRAGMENT

// Inputs from previous shader
varying vec2 texturePos;

// The texture
uniform sampler2D inputTexture;

// Parameters        
uniform float paramAmbientLightIntensity;

void main()
{
    gl_FragColor = texture2D(inputTexture, texturePos) * paramAmbientLightIntensity;
} 
