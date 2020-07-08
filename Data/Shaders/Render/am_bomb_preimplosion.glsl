###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inAMBombPreImplosion1;  // Position, CenterPosition
in vec2 inAMBombPreImplosion2; // Progress, Radius

// Outputs
out vec2 vertexWorldCoords;
out vec2 vertexWorldCenterPosition;
out float vertexProgress;
out float vertexWorldRadius;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexWorldCoords = inAMBombPreImplosion1.xy;
    vertexWorldCenterPosition = inAMBombPreImplosion1.zw;

    vertexProgress = inAMBombPreImplosion2.x;
    vertexWorldRadius = inAMBombPreImplosion2.y;

    gl_Position = paramOrthoMatrix * vec4(inAMBombPreImplosion1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexWorldCoords;
in vec2 vertexWorldCenterPosition;
in float vertexProgress;
in float vertexWorldRadius;

void main()
{
    float d = distance(vertexWorldCoords, vertexWorldCenterPosition) / vertexWorldRadius;

    if (d > 1.0f)
        discard;
    /*
    float progress = vertexProgress - 0.5; // (-0.5, 0.5]

    // No rotation for the time being
    // float angle = progress;

    // Calculate fragment's coordinates in the NDC space
    vec2 ndc = (gl_FragCoord.xy / paramViewportSize.xy) * 2.0 - vec2(1.0);

    // Center and flip vertically
    ndc = vec2(ndc.x - vertexCenterPosition.x, vertexCenterPosition.y - ndc.y);

    // ------------------    
    */

    // TODOHERE
    gl_FragColor = vec4(1.0, 1.0, 1.0, d);
}
