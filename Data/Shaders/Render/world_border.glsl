###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inWorldBorder; // Position (vec2), TextureCoordinates (vec2)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inWorldBorder.xy, -1.0, 1.0);
    texturePos = inWorldBorder.zw;
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 texturePos;

// The texture
uniform sampler2D paramWorldBorderTexture;

// Parameters        
uniform float paramAmbientLightIntensity;

void main()
{
    vec4 textureColor = texture2D(paramWorldBorderTexture, texturePos);
    gl_FragColor = vec4(textureColor.xyz * paramAmbientLightIntensity, 0.75);
} 
