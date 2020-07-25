###VERTEX

#version 120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inTextureNotification1; // Position (vec2), TextureCoordinates (vec2)
in float inTextureNotification2; // Alpha

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;

void main()
{
    vertexTextureCoordinates = inTextureNotification1.zw; 
    vertexAlpha = inTextureNotification2;
    gl_Position = vec4(inTextureNotification1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;

// Params
uniform float paramTextureLighteningStrength;

// The texture
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

void main()
{
    vec4 textureColor = texture2D(paramGenericLinearTexturesAtlasTexture, vertexTextureCoordinates);

    vec3 textureColor2 = mix(textureColor.xyz, vec3(0.8, 0.8, 0.8), paramTextureLighteningStrength);

    gl_FragColor = vec4(
        textureColor2,
        textureColor.w * vertexAlpha);
} 
