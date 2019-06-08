###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inOcean;	// Position (vec2), Texture coordinate Y (float) (0 at top, +X at bottom)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec3 textureCoord;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOcean.xy, -1.0, 1.0);
    textureCoord = inOcean;
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec3 textureCoord;

// The texture
uniform sampler2D paramOceanTexture;

// Parameters        
uniform float paramAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec2 paramTextureScaling;
uniform float paramOceanDarkeningRate;

void main()
{
    float increment = -exp(-textureCoord.z) * 10.0;
    vec2 textureCoord2 = vec2(textureCoord.x, textureCoord.z + increment);

    float darkMix = 1.0 - exp(min(0.0, textureCoord.y) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec4 textureColor = mix(
        texture2D(paramOceanTexture, textureCoord2 * paramTextureScaling),
        vec4(0,0,0,0), 
        pow(darkMix, 3.0));

    gl_FragColor = vec4(textureColor.xyz * paramAmbientLightIntensity, 1.0 - paramOceanTransparency);
} 
