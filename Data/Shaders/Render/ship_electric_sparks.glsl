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
    float depth = 1.0 - smoothstep(0.0, 0.5, abs(vertexGamma - 0.5));

    // TODOTEST
    //if (depth < 0.2)
    //    discard;

    gl_FragColor = vec4(
        vec3(0.6, 0.8, 1.0), 
        depth);
} 
