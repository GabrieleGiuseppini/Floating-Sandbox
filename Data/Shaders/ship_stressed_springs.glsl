###VERTEX

// Inputs
attribute vec2 inputPos;

// Outputs        
varying vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoords = inputPos; 
    gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
}

###FRAGMENT

// Inputs
varying vec2 vertexTextureCoords;

// Input texture
uniform sampler2D inputTexture;

void main()
{
    gl_FragColor = texture2D(inputTexture, vertexTextureCoords);
} 
