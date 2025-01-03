###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inMultiNotification1;  // Type, WorldPosition (vec2)
in vec2 inMultiNotification2; // VirtualSpacePosition (vec2)

// Outputs
out float type;
out vec2 virtualSpacePosition;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    type = inMultiNotification1.x;
    virtualSpacePosition = inMultiNotification2.xy;

    gl_Position = paramOrthoMatrix * vec4(inMultiNotification1.yz, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float type;
in vec2 virtualSpacePosition; // [-1.0, 1.0]

// TODOHERE
// The texture
//uniform sampler2D paramNoiseTexture;

void main()
{
    float d = length(virtualSpacePosition);

    #define MaxAlpha 0.5
    float alpha = MaxAlpha * (1.0 - smoothstep(0.95, 1.0, d));

    gl_FragColor = vec4(0.0, 0.0, 0.0, alpha);
}
