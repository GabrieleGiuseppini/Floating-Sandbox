###VERTEX

#version 130

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inSharedAttribute1; // Position (vec2), TextureCoordinates (vec2)
in float inSharedAttribute2; // alpha

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;

void main()
{
    vertexTextureCoordinates = inSharedAttribute1.zw; 
    vertexAlpha = inSharedAttribute2;
    gl_Position = vec4(inSharedAttribute1.xy, -1.0, 1.0);
}


###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;

// The texture
uniform sampler2D inputTexture;

void main()
{
    vec4 textureColor = texture2D(inputTexture, vertexTextureCoordinates);

    gl_FragColor = vec4(
	textureColor.xyz,
	textureColor.w * vertexAlpha);
} 
