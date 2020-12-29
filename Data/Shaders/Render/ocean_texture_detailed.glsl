###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inOceanDetailed1;	// TODO: Position (vec2), Texture coordinate Y (float) (0 at top, +X at bottom)
in vec2 inOceanDetailed2;	// TODO

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec3 textureCoord;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1.xy, -1.0, 1.0);
    textureCoord = inOceanDetailed1.xyz;
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec3 textureCoord;

// The texture
uniform sampler2D paramOceanTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec2 paramTextureScaling;
uniform float paramOceanDarkeningRate;

void main()
{
    // Back-sample 5.0 at top, slowly going to zero
    float sampleIncrement = -5.0 * (2.0 - 2.0 * smoothstep(-1.0, 1.0, textureCoord.z));
    vec2 textureCoord2 = vec2(textureCoord.x, textureCoord.z + sampleIncrement);

    float darkMix = 1.0 - exp(min(0.0, textureCoord.y) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec4 textureColor = mix(
        texture2D(paramOceanTexture, textureCoord2 * paramTextureScaling),
        vec4(0,0,0,0), 
        pow(darkMix, 3.0));

    // Lighten the top of the water
    textureColor *= 1.0 + (1.0 - smoothstep(0.0, 1.0, textureCoord.z)) * 0.1;

    gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity, 1.0 - paramOceanTransparency);
} 
