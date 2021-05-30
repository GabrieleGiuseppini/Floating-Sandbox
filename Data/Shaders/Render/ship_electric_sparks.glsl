###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inElectricSpark1; // Position (vec2), PlaneId, Gamma

// Outputs        
out float vertexGamma;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexGamma = inElectricSpark1.w;
    gl_Position = paramOrthoMatrix * vec4(inElectricSpark1.xyz, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader        
in float vertexGamma;

void main()
{
    float d = abs(vertexGamma - 0.5);

    float depthWhite = 1.0 - smoothstep(0.0, 0.15, d);
    float depth = 1.0 - smoothstep(0.0, 0.5, d);

    gl_FragColor = vec4(
        mix(
            vec3(0.6, 0.8, 1.0), 
            vec3(1.),
            depthWhite),
        depth);
} 
