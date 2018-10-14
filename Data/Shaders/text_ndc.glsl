###VERTEX

#version 130

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec2 inSharedPosition;
in vec2 inSharedTextureCoordinates;
in float inShared1XFloat; // alpha

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;

void main()
{
    vertexTextureCoordinates = inSharedTextureCoordinates; 
    vertexAlpha = inShared1XFloat;
    gl_Position = vec4(inSharedPosition.xy, -1.0, 1.0);
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
