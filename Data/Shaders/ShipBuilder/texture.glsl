###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inTexture; // Vertex position (ship space), Texture coords (texture space)

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
uniform sampler2D paramTextureUnit1;

// Params
uniform float paramOpacity;

void main()
{
    vec4 sample = texture2D(paramTextureUnit1, vertexTextureCoordinates);
    gl_FragColor = vec4(sample.xyz, sample.w * paramOpacity);
} 
