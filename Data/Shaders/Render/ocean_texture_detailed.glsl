###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inOceanDetailed1;	// Position (vec2)
in vec4 inOceanDetailed2;	// yTexture (float), yBack/yMid/yFront (float, world y)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 worldCoords;
out vec4 yVector;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inOceanDetailed1, -1.0, 1.0);
    worldCoords = inOceanDetailed1;
    yVector = inOceanDetailed2;    
}


###FRAGMENT

#version 120

#include "ocean_surface.glslinc"

#define in varying

// Inputs from previous shader
in vec2 worldCoords;
in vec4 yVector;

// The texture
uniform sampler2D paramOceanTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanTransparency;
uniform vec2 paramTextureScaling;
uniform float paramOceanDarkeningRate;
uniform float paramOceanSurfaceBackPlaneToggle;

void main()
{
    // Get texture color sample
    vec2 textureCoord2 = vec2(worldCoords.x, yVector.x);
    float darkMix = 1.0 - exp(min(0.0, worldCoords.y) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec3 textureColor = mix(
        texture2D(paramOceanTexture, textureCoord2 * paramTextureScaling).xyz,
        vec3(0.), 
        pow(darkMix, 3.0));

    // Apply detail
    vec4 color = CalculateOceanPlaneColor(
        vec4(textureColor, (1.0 - paramOceanTransparency)), 
        worldCoords.y,
        yVector.y, yVector.z, yVector.w, 
        paramOceanSurfaceBackPlaneToggle);

    // Combine
    gl_FragColor = vec4(color.xyz * paramEffectiveAmbientLightIntensity, color.w);
} 
