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

    #define WhiteWidth .1
    float depthWhite = 1.0 - min(d, WhiteWidth) / WhiteWidth;
    #define AlphaWidth .45
    float alpha = 1.0 - min(d, AlphaWidth) / AlphaWidth;

    gl_FragColor = vec4(
        mix(
            vec3(0.6, 0.8, 1.0), 
            vec3(1.),
            depthWhite),
        alpha);
} 
