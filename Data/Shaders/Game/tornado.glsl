###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inTornado1;  // Position, TornadoSpaceCoords
in vec3 inTornado2; // RotationSpeedMultiplier, HeatDepth, VisibilityAlpha

// Outputs
out vec2 vertexTornadoSpaceCoords;
out float vertexRotationSpeedMultiplier;
out float vertexHeatDepth;
out float vertexVisibilityAlpha;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTornadoSpaceCoords = inTornado1.zw;
    vertexRotationSpeedMultiplier = inTornado2.x;
    vertexHeatDepth = inTornado2.y;
    vertexVisibilityAlpha = inTornado2.z;

    gl_Position = paramOrthoMatrix * vec4(inTornado1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTornadoSpaceCoords;
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
