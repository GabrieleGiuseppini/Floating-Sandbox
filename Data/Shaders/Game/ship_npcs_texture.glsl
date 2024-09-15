###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inNpcTextureAttributeGroup1; // Position, TextureCoords
in vec3 inNpcTextureAttributeGroup2; // PlaneId
in vec4 inNpcTextureAttributeGroup3; // OverlayColor

// Outputs
out float vertexWorldY;
out vec2 textureCoords;
out vec4 vertexOverlayColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexWorldY = inNpcTextureAttributeGroup1.y;
    textureCoords = inNpcTextureAttributeGroup1.zw;
    vertexOverlayColor = inNpcTextureAttributeGroup3;

    gl_Position = paramOrthoMatrix * vec4(inNpcTextureAttributeGroup1.xy, inNpcTextureAttributeGroup2.x, 1.0);
}

###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"

// Inputs from previous shader
in float vertexWorldY;
in vec2 textureCoords;
in vec4 vertexOverlayColor;

// The texture
uniform sampler2D paramNpcAtlasTexture;

// Params
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanDepthDarkeningRate;
uniform float paramShipDepthDarkeningSensitivity;

void main()
{
    // Sample texture
    vec4 c = texture2D(paramNpcAtlasTexture, textureCoords);

    // Fragments with alpha lower than this are discarded
    #define MinAlpha 0.2
    if (c.a < MinAlpha) // We don't Z-sort NPCs
        discard;   

    // Apply highlight
    c = vec4(
        mix(
            c.rgb,
            vertexOverlayColor.rgb,
            vertexOverlayColor.a * c.a * 0.8),
        c.a);

    // Calculate lamp tool intensity
    float lampToolIntensity = CalculateLampToolIntensity(gl_FragCoord.xy);

    if (paramShipDepthDarkeningSensitivity > 0.0) // Fine to branch - all pixels will follow the same branching
    {
        // Calculate depth darkening
        float darkeningFactor = CalculateOceanDepthDarkeningFactor(
            vertexWorldY,
            paramOceanDepthDarkeningRate);

        // Apply depth darkening
        c.rgb = mix(
            c.rgb,
            vec3(0.),
            darkeningFactor * (1.0 - lampToolIntensity) * paramShipDepthDarkeningSensitivity);
    }
    
    // Apply ambient light
    c.rgb *= max(paramEffectiveAmbientLightIntensity, lampToolIntensity);        

    gl_FragColor = c;
} 
