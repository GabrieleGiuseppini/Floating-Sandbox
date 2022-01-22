###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inWindSphere1;  // VertexPosition (world), CenterPosition (world)
in vec4 inWindSphere2;  // PreFrontRadius (world), PreFrontIntensity, MainFrontRadius (world), MainFrontIntensity
in float inWindSphere3; // ZeroFrontRadius (world)

// Outputs
out vec2 vertexPositionWorld;
out vec2 centerPositionWorld;
out float preFrontRadiusWorld;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexPositionWorld = inWindSphere1.xy;
    centerPositionWorld = inWindSphere1.zw;
    preFrontRadiusWorld = inWindSphere2.x;

    gl_Position = paramOrthoMatrix * vec4(vertexPositionWorld, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexPositionWorld;
in vec2 centerPositionWorld;
in float preFrontRadiusWorld;

// Parameters
uniform float paramTime;

void main()
{
    float radiusWorld = length(vertexPositionWorld - centerPositionWorld);

    radiusWorld *= min(max(paramTime, 1.0), 1.0);

    float whiteDepth = 
        (1.0 - step(preFrontRadiusWorld, radiusWorld))
        * step(preFrontRadiusWorld - 5.0, radiusWorld);
    
    ///////

    gl_FragColor = vec4(whiteDepth, whiteDepth, whiteDepth, 1.);
}
