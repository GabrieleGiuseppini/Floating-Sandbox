###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inShipPointAttributeGroup1; // Position, TextureCoordinates
in vec4 inShipPointAttributeGroup2; // Light, Water, PlaneId, Decay

// Outputs        
out vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoords = inShipPointAttributeGroup1.xy; 
    gl_Position = paramOrthoMatrix * vec4(inShipPointAttributeGroup1.xy, inShipPointAttributeGroup2.z, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs
in vec2 vertexTextureCoords;

// Input texture
uniform sampler2D sharedSpringTexture;

void main()
{
    gl_FragColor = texture2D(sharedSpringTexture, vertexTextureCoords);
} 
