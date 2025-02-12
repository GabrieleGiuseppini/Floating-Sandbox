###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inVertexShaderInput0; // NDC world (x,y); texture coords (z,w)

// Outputs
out vec2 textureCoords;

void main()
{
    gl_Position = vec4(inVertexShaderInput0.xy, -1.0, 1.0);
    textureCoords = inVertexShaderInput0.zw;
}


###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 textureCoords;

// Input textures
uniform sampler2D paramTextureInput0;
uniform sampler2D paramTextureInput1;

void main()
{
    vec4 a = texture2D(paramTextureInput0, textureCoords);
    vec4 b = texture2D(paramTextureInput1, textureCoords);
    gl_FragColor = a + b;
} 
