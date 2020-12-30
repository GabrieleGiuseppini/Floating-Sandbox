###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inOceanDetailed1;	// Position (vec2)
in vec4 inOceanDetailed2;	// yWater, yBack, yMid, yFront (float: 0.0 at top, +foo at bottom)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec4 yWaters;
out vec2 textureCoord;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1, -1.0, 1.0);
    yWaters = inOceanDetailed2;
    textureCoord = inOceanDetailed1;
}


###FRAGMENT

#version 120

#include "ocean_surface.glslinc"

#define in varying

// Inputs from previous shader
in vec4 yWaters;
in vec2 textureCoord;

// The texture
uniform sampler2D paramOceanTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec2 paramTextureScaling;
uniform float paramOceanDarkeningRate;

void main()
{
    // Get texture color sample
    vec2 textureCoord2 = vec2(textureCoord.x, yWaters.x);
    float darkMix = 1.0 - exp(min(0.0, textureCoord.y) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec3 textureColor = mix(
        texture2D(paramOceanTexture, textureCoord2 * paramTextureScaling).xyz,
        vec3(0.), 
        pow(darkMix, 3.0));

    // Apply detail
    vec4 color = CalculateOceanPlaneColor(textureColor, yWaters.x, yWaters.y, yWaters.z, yWaters.w, 1.);

    // Combine
    gl_FragColor = vec4(color.xyz * paramEffectiveAmbientLightIntensity, color.w * (1.0 - paramOceanTransparency));
} 
