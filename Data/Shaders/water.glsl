###VERTEX

#version 130

// Inputs
in vec2 inWaterPosition;
in float inSharedAttribute0; // texture coordinate Y;

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inWaterPosition.xy, -1.0, 1.0);
    texturePos = vec2(inWaterPosition.x, inSharedAttribute0);
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
