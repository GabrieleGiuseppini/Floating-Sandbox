###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inFlame1; // Position, PlaneId
in vec2 inFlame2; // FlameSpacePosition (x=[-0.5, 0.5], y=[0.0, 1.0])

// Outputs
out vec2 flameSpacePosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    flameSpacePosition = inFlame2.xy;

    gl_Position = paramOrthoMatrix * vec4(inFlame1.xyz, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 flameSpacePosition;

// Params
uniform float paramTime;
uniform float paramWindSpeedMagnitude;

mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);
    return m;
}

#define FlameSpeed 0.17
#define NoiseResolution 0.4

void main()
{
    // TODOHERE
    float d = length(flameSpacePosition);
    d = d + paramTime + paramWindSpeedMagnitude;
    d = d - paramTime - paramWindSpeedMagnitude;
    vec3 col = mix(vec3(0.8, 0.1, 0.1), vec3(0.1, 0.1, 0.8), d);
    gl_FragColor = vec4(col.xyz, 1.0);
} 
