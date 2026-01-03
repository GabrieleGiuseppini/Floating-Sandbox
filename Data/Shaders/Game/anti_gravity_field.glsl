###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inAntiGravityField1;  // Position, FieldSpaceCoords
in float inAntiGravityField2; // worldXExtent

// Outputs
out vec2 vertexFieldSpaceCoords;
out float vertexWorldXExtent;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexFieldSpaceCoords = inAntiGravityField1.zw;
    vertexWorldXExtent = inAntiGravityField2;

    gl_Position = paramOrthoMatrix * vec4(inAntiGravityField1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexFieldSpaceCoords;
in float vertexWorldXExtent;

// Textures
uniform sampler2D paramNoiseTexture;

// Params
uniform float paramSimulationTime;

void main()
{
    // TODOHERE
    #define NOISE_RESOLUTION 1.0
    vec2 noiseCoords = vec2(vertexWorldXExtent + paramSimulationTime / 5.0, vertexFieldSpaceCoords.y) * NOISE_RESOLUTION;
    float noiseSample = texture2D(paramNoiseTexture, noiseCoords).x;
    gl_FragColor = vec4(noiseSample, noiseSample, noiseSample, 1.0);
}
