###VERTEX

#version 130

// Inputs
in vec2 shipPointPosition;

// Outputs        
out vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoords = shipPointPosition; 
    gl_Position = paramOrthoMatrix * vec4(shipPointPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Inputs
in vec2 vertexTextureCoords;

// Input texture
uniform sampler2D inputTexture;

void main()
{
    gl_FragColor = texture2D(inputTexture, vertexTextureCoords);
} 
