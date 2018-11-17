###VERTEX

#version 130

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inSharedAttribute0; // Position (vec2), TextureCoordinates (vec2)
in float inSharedAttribute1; // alpha

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;

void main()
{
    vertexTextureCoordinates = inSharedAttribute0.zw; 
    vertexAlpha = inSharedAttribute1;
    gl_Position = vec4(inSharedAttribute0.xy, -1.0, 1.0);
}


###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;

// The texture
uniform sampler2D sharedSpringTexture;

void main()
{
    vec4 textureColor = texture2D(sharedSpringTexture, vertexTextureCoordinates);

    gl_FragColor = vec4(
        textureColor.xyz,
        textureColor.w * vertexAlpha);
} 
