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

// Inputs from previous shader
in vec2 textureSpaceCoords; // 3.0 => 3 full frames

// The texture
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

// Parameters        
uniform vec2 paramAtlasTile1Dx; // span across two pixels
uniform vec2 paramAtlasTile1LeftBottomTextureCoordinates;
uniform vec2 paramAtlasTile1Size;
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    //
    // Wrap the atlas texture tile
    //

    // Clamp to fight against linear filtering - though no idea why dx works and dx/2.0 (half pixel) doesn't
    vec2 uv = clamp(
        fract(textureSpaceCoords), 
        paramAtlasTile1Dx, 
        vec2(1.0) - paramAtlasTile1Dx);

    vec2 sampleCoords = paramAtlasTile1LeftBottomTextureCoordinates + paramAtlasTile1Size * uv;

    vec4 textureColor = texture2D(paramGenericLinearTexturesAtlasTexture, sampleCoords);
    gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity, 0.75);
} 
