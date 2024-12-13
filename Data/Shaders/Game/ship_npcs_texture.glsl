###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inNpcAttributeGroup1; // Position
in vec4 inNpcAttributeGroup2; // PlaneId, Alpha, HighlightAlpha, RemovalProgress
in vec3 inNpcAttributeGroup3; // VertexTextureCoords, Light

// Outputs
out float vertexWorldY;
out float vertexAlpha;
out float vertexHighlightAlpha;
out float vertexRemovalProgress;
out vec2 vertexTextureCoords;
out float vertexLight;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexWorldY = inNpcAttributeGroup1.y;
    vertexAlpha = inNpcAttributeGroup2.y;
    vertexHighlightAlpha = inNpcAttributeGroup2.z;
    vertexRemovalProgress = inNpcAttributeGroup2.w;
    vertexTextureCoords = inNpcAttributeGroup3.xy;
    vertexLight = inNpcAttributeGroup3.z;

    gl_Position = paramOrthoMatrix * vec4(inNpcAttributeGroup1.xy, inNpcAttributeGroup2.x, 1.0);
}

###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"
#include "static_parameters.glslinc"

// Inputs from previous shader
in float vertexWorldY;
in float vertexAlpha;
in float vertexHighlightAlpha;
in float vertexRemovalProgress;
in vec2 vertexTextureCoords;
in float vertexLight;

// The texture
uniform sampler2D paramNpcAtlasTexture;

// Params
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramLampLightColor;
uniform float paramOceanDepthDarkeningRate;
uniform float paramShipDepthDarkeningSensitivity;

void main()
{
    // Sample texture
    vec4 c = texture2D(paramNpcAtlasTexture, vertexTextureCoords);

    // Fragments with alpha lower than this are discarded
    #define MinAlpha 0.2
    if (c.a < MinAlpha) // We don't Z-sort NPCs
        discard;   

    // Apply highlight (overlay blending mode)
    //
    // (Target > 0.5) * (1 – (1-2*(Target-0.5)) * (1-Blend)) +
    // (Target <= 0.5) * lerp(Target, Blend, alphaMagic)
    
    vec3 overlayColor = vec3(NPC_HIGHLIGHT_COLOR);
    vec3 IsTargetLarge = step(vec3(0.5), c.rgb);
    vec3 TargetHigh = 1. - (1. - (c.rgb - .5) * 2.) * (1. - overlayColor.rgb);
    vec3 TargetLow = mix(c.rgb, overlayColor.rgb, 0.6);
    
    vec3 ovCol =
        IsTargetLarge * TargetHigh
        + (1. - IsTargetLarge) * TargetLow;

    c.rgb = mix(
        c.rgb,
        ovCol,
        vertexHighlightAlpha * c.a);

    // Calculate lamp tool intensity
    float lampToolIntensity = CalculateLampToolIntensity(gl_FragCoord.xy);

    // Calculate removal shading depth
    float lum = 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
    float removalDepth = min(lum + vertexRemovalProgress, 1.0);
    removalDepth *= vertexRemovalProgress;
    vec3 c2 = min(
        vec3(NPC_REMOVAL_COLOR),
        c.rgb + vec3(NPC_REMOVAL_COLOR) * removalDepth);

    c.rgb = mix(
        c.rgb,
        c2,
        removalDepth);
    c.a = mix(
        c.a,
        rand(vertexTextureCoords * vertexRemovalProgress),
        vertexRemovalProgress);

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
            darkeningFactor * (1.0 - lampToolIntensity) * (1.0 - vertexLight) * (1.0 - removalDepth) * paramShipDepthDarkeningSensitivity);
    }
    
    // Apply ambient/lamp light
    // (complement missing ambient light with point's light)
    c.rgb *= max(
        mix(
            vertexLight,
            1.0, 
            max(paramEffectiveAmbientLightIntensity, removalDepth)),
        lampToolIntensity);

    // Apply electric lamp color
    c.rgb = mix(c.rgb, paramLampLightColor, vertexLight);

    c.a *= vertexAlpha;

    gl_FragColor = c;
} 
