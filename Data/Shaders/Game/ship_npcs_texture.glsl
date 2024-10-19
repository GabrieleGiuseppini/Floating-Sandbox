###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inNpcAttributeGroup1; // Position
in vec4 inNpcAttributeGroup2; // PlaneId, OverlayColor
in vec2 inNpcAttributeGroup3; // VertexSpacePosition

// Outputs
out float vertexWorldY;
out vec2 textureCoords;
out vec3 vertexOverlayColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexWorldY = inNpcAttributeGroup1.y;
    textureCoords = inNpcAttributeGroup3;
    vertexOverlayColor = inNpcAttributeGroup2.yzw;

    gl_Position = paramOrthoMatrix * vec4(inNpcAttributeGroup1.xy, inNpcAttributeGroup2.x, 1.0);
}

###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"

// Inputs from previous shader
in float vertexWorldY;
in vec2 textureCoords;
in vec3 vertexOverlayColor;

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

    // Apply highlight (overlay blending mode)
    //
    // (Target > 0.5) * (1 – (1-2*(Target-0.5)) * (1-Blend)) +
    // (Target <= 0.5) * lerp(Target, Blend, alphaMagic)
    
    vec3 IsTargetLarge = step(vec3(0.5), c.rgb);
    vec3 TargetHigh = 1. - (1. - (c.rgb - .5) * 2.) * (1. - vertexOverlayColor.rgb);
    vec3 TargetLow = mix(c.rgb, vertexOverlayColor.rgb, 0.6);
    
    vec3 ovCol =
        IsTargetLarge * TargetHigh
        + (1. - IsTargetLarge) * TargetLow;

    c.rgb = mix(
        c.rgb,
        ovCol,
        step(0.0001, vertexOverlayColor.r + vertexOverlayColor.g + vertexOverlayColor.b) * c.a);

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
