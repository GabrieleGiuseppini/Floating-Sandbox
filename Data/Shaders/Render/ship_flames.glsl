###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inFlame1; // Position, PlaneId
in vec2 inFlame2; // BaseCenterPosition

// Outputs
out vec2 vertexCenterPosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexCenterPosition = 
        (paramOrthoMatrix * vec4(inFlame2.xy, -1.0, 1.0)).xy;

    gl_Position = paramOrthoMatrix * vec4(inFlame1.xyz, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexCenterPosition;

// Params
uniform float paramTime;
uniform vec2 paramViewportSize;
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
    // Calculate fragment's coordinates in the pseudo-NDC space (-0.5, +0.5)
    vec2 ndc = (gl_FragCoord.xy / paramViewportSize.xy) - vec2(0.5);

    // Center and flip vertically
    ndc = vec2(ndc.x - vertexCenterPosition.x / 2.0, vertexCenterPosition.y / 2.0 - ndc.y);

    // Adjust for aspect ratio
    ndc.y /= (paramViewportSize.x / paramViewportSize.y);

    // TODOHERE
    float d = length(ndc);
    d = d + paramTime + paramWindSpeedMagnitude;
    d = d - paramTime - paramWindSpeedMagnitude;
    vec3 col = mix(vec3(0.8, 0.1, 0.1), vec3(0.1, 0.1, 0.8), 1.0 - smoothstep(0.0, 0.05, d));
    gl_FragColor = vec4(col.xyz, 1.0);
} 
