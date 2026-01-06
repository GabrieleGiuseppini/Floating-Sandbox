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
    // Max distance: 1.0
    float d = length(vertexFieldSpaceCoords - vec2(0.5)) * 2.0;
    float alpha = 1.0 - smoothstep(0.5, 1.0, d);
    
    //
    // Zero-centered y coords
    //
    
    // 0..1
    float vertexFieldSpaceYCentered = abs(0.5  - vertexFieldSpaceCoords.y) / 0.5;
    
    //
    // Calculate perturbation
    //
    
    #define PERTURBATION_RESOLUTION 0.01
    #define PERTURBATION_SPEED 0.009
    
    vec2 perturbationCoords = vec2(
        vertexWorldXExtent * PERTURBATION_RESOLUTION - paramSimulationTime * PERTURBATION_SPEED, 
        paramSimulationTime * PERTURBATION_SPEED);
    float perturbationNoise = texture2D(paramNoiseTexture, perturbationCoords).r; // 0..1
    float perturbationAngle = 0.8 * (perturbationNoise * 2.0 - 1.0);    
    float vertexWorldXExtentPerturbated = vertexWorldXExtent + perturbationAngle * vertexFieldSpaceYCentered;
    
    
    //
    // Calculate noise coords
    //
    
    #define H_RESOLUTION 50.0
    #define SPEED 1.0 / 30.0
    
    float noiseYFactor = 0.15;
    
    float bottomHalfXOffset = step(vertexFieldSpaceCoords.y, 0.5) * 0.5;
    float noiseY = vertexFieldSpaceYCentered * noiseYFactor;
    float compressedNoiseY = smoothstep(0.0, 1.0, noiseY * 0.5) / 0.5;
    //float compressedNoiseY = noiseY;
    
    vec2 noiseCoords = vec2(
        vertexWorldXExtentPerturbated / H_RESOLUTION + bottomHalfXOffset,
        compressedNoiseY - paramSimulationTime * SPEED);
    float noise = texture2D(paramNoiseTexture, noiseCoords).x;
    
    // Add center h line
    noise = mix(1.0, noise, vertexFieldSpaceYCentered);
    
    // Focus
    noise = pow(noise, 7.0);
    
    ///////////////////////////////////////////////
    
    // glBlendFunc(GL_ONE, GL_ONE)
    gl_FragColor = vec4(vec3(noise*alpha), 1.);
}
