###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in float inShipPointPlaneId;

// Outputs        
out vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoords = inShipPointPosition;  // Using world pos
    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition, inShipPointPlaneId, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs
in vec2 vertexTextureCoords;

// Input texture
uniform sampler2D paramSharedTexture;

void main()
{
    gl_FragColor = texture2D(paramSharedTexture, vertexTextureCoords);
} 
