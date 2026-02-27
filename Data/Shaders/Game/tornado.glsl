###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inTornado1;  // Position, TornadoSpaceCoords
in vec4 inTornado2; //  BottomWidthFraction, RotationSpeedMultiplier, HeatDepth, VisibilityAlpha

// Outputs
out vec2 vertexTornadoSpaceCoords;
out float vertexBottomWidthFraction;
out float vertexRotationSpeedMultiplier;
out float vertexHeatDepth;
out float vertexVisibilityAlpha;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTornadoSpaceCoords = inTornado1.zw;
    vertexBottomWidthFraction = inTornado2.x;
    vertexRotationSpeedMultiplier = inTornado2.y;
    vertexHeatDepth = inTornado2.z;
    vertexVisibilityAlpha = inTornado2.w;

    gl_Position = paramOrthoMatrix * vec4(inTornado1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTornadoSpaceCoords;
in float vertexBottomWidthFraction;
in float vertexRotationSpeedMultiplier;
in float vertexHeatDepth;
in float vertexVisibilityAlpha;

// Textures
uniform sampler2D paramNoiseTexture;
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

// Params
uniform float paramSimulationTime;

void main()
{
    // TODOHERE
    gl_FragColor = vec4(
        vec3(
            vertexHeatDepth,
            clamp(vertexRotationSpeedMultiplier - 1., 0., 1.),
            paramSimulationTime * 0.000000001), 
        0.5 * vertexVisibilityAlpha);
}
