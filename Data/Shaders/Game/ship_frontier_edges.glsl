###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inShipPointAttributeGroup1; // Position, TextureCoordinates
in vec4 inShipPointAttributeGroup2; // Light, Water, PlaneId, Decay
in vec4 inShipPointFrontierColor; // FrontierBaseColor (vec3), PositionalProgress (float)

// Params
uniform float paramTime;

// Outputs        
out vec3 vertexFrontierBaseColor;
out float vertexPositionalProgress;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexFrontierBaseColor = inShipPointFrontierColor.xyz;
    vertexPositionalProgress = inShipPointFrontierColor.w - paramTime * 4.;
    gl_Position = paramOrthoMatrix * vec4(inShipPointAttributeGroup1.xy, inShipPointAttributeGroup2.z, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs
in vec3 vertexFrontierBaseColor;
in float vertexPositionalProgress;

void main()
{
    float progress = 1. + sin(2. * 3.1415 / 1. * vertexPositionalProgress) / 2.;

    gl_FragColor = vec4(vertexFrontierBaseColor * progress,1.0);
} 
