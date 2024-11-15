###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inWorldBorder; // Position (vec2), TextureSpaceCoords (vec2)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 textureSpaceCoords;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inWorldBorder.xy, -1.0, 1.0);
    textureSpaceCoords = inWorldBorder.zw;
}

###FRAGMENT-120

#define in varying

#include "static_parameters.glslinc"

// Inputs from previous shader
in vec2 textureSpaceCoords; // 3.0 => 3 full frames

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    // 0.0 on hard border
    float borderValue = min(textureSpaceCoords.x, textureSpaceCoords.y);
    float alpha = 1.0 - borderValue;

    gl_FragColor = vec4(
        vec3(STOCK_COLOR_RED1) * paramEffectiveAmbientLightIntensity,
        alpha);
} 
