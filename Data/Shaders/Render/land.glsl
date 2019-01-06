###VERTEX

#version 130

// Inputs
in vec2 inSharedAttribute1; // Position

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inSharedAttribute1.xy, -1.0, 1.0);
    texturePos = inSharedAttribute1.xy;
}

###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 texturePos;

// The texture
uniform sampler2D paramLandTexture;

// Parameters        
uniform float paramAmbientLightIntensity;
uniform vec2 paramTextureScaling;

void main()
{
    vec4 textureColor = texture2D(paramLandTexture, texturePos * paramTextureScaling);
    gl_FragColor = vec4(textureColor.xyz * paramAmbientLightIntensity, 1.0);
} 
