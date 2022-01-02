###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inTextureNdc; // Vertex position (NDC), Texture coords (texture space)

// Outputs
out vec2 vertexTextureCoordinates;

void main()
{
    vertexTextureCoordinates = inTextureNdc.zw; 
    
    gl_Position = vec4(inTextureNdc.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;

// The texture
uniform sampler2D paramBackgroundTextureUnit;

void main()
{
    gl_FragColor = texture2D(paramBackgroundTextureUnit, vertexTextureCoordinates);
} 
