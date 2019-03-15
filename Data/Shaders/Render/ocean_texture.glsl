###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inOcean;	// Position (vec2), Texture coordinate Y (float)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOcean.xy, -1.0, 1.0);
    texturePos = inOcean.xz;
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 texturePos;

// The texture
uniform sampler2D paramOceanTexture;

// Parameters        
uniform float paramAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec2 paramTextureScaling;

void main()
{
    vec4 textureColor = texture2D(paramOceanTexture, texturePos * paramTextureScaling);
    gl_FragColor = vec4(textureColor.xyz * paramAmbientLightIntensity, 1.0 - paramOceanTransparency);
} 
