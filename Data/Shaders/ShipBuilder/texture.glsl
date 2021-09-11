###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inTexture; // Vertex position (work space), Texture coords (texture space)

// Outputs
out vec2 vertexTextureCoordinates;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inTexture.zw; 
    
    gl_Position = paramOrthoMatrix * vec4(inTexture.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;

// The texture
uniform sampler2D paramTexture1;

void main()
{
    gl_FragColor = texture2D(paramTexture1, vertexTextureCoordinates);
} 
