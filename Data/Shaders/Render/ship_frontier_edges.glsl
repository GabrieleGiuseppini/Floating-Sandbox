###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inShipPointAttributeGroup1; // Position, TextureCoordinates
in vec4 inShipPointAttributeGroup2; // Light, Water, PlaneId, Decay
in vec4 inShipPointFrontierColor; // FrontierBaseColor (vec3), PositionalProgress (float)

// Outputs        
out vec3 vertexFrontierBaseColor;
out float vertexPositionalProgress;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexFrontierBaseColor = inShipPointFrontierColor.xyz;
    vertexPositionalProgress = inShipPointFrontierColor.w;
    gl_Position = paramOrthoMatrix * vec4(inShipPointAttributeGroup1.xy, inShipPointAttributeGroup2.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs
in vec3 vertexFrontierBaseColor;
in float vertexPositionalProgress;

void main()
{
    // TODO
    gl_FragColor = vec4(vertexFrontierBaseColor, 1.0);
} 
