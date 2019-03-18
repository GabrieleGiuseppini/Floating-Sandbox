###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in vec3 inShipPointAttributeGroup1; // Light, Water, PlaneId

// Outputs        
out vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoords = inShipPointPosition; 
    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, inShipPointAttributeGroup1.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs
in vec2 vertexTextureCoords;

// Input texture
uniform sampler2D sharedSpringTexture;

void main()
{
    gl_FragColor = texture2D(sharedSpringTexture, vertexTextureCoords);
} 
