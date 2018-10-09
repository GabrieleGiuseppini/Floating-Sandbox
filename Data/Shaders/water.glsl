###VERTEX

#version 130

// Inputs
in vec2 waterPosition;
in float shared1XFloat; // texture coordinate Y;

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(waterPosition.xy, -1.0, 1.0);
    texturePos = vec2(waterPosition.x, shared1XFloat);
}


###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 texturePos;

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
