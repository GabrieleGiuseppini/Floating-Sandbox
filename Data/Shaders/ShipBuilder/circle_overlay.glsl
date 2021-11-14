###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inCircleOverlay1; // Vertex position (ship space), Normalized coords (0->1)
in vec3 inCircleOverlay2; // Color

// Outputs
out vec2 normalizedCoords;
out vec3 circleBorderColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    normalizedCoords = inCircleOverlay1.zw;
    circleBorderColor = inCircleOverlay2;
    gl_Position = paramOrthoMatrix * vec4(inCircleOverlay1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 normalizedCoords; // (x=[0.0, 1.0], y=[0.0, 1.0])
in vec3 circleBorderColor;

// Params
uniform vec2 paramPixelSize;

void main()
{
    #define SafetyMultiplier 1.25

    float d = length(normalizedCoords - vec2(0.5));

    // Codework: do we need to use bi-dimensional pixel size?
    float smoothWidth = paramPixelSize.x * 1.5;
    float borderDepth = 
        smoothstep(0.5 - paramPixelSize.x * SafetyMultiplier - 2.0 * smoothWidth, 0.5 - paramPixelSize.x * SafetyMultiplier - smoothWidth, d)
        - smoothstep(0.5 - paramPixelSize.x * SafetyMultiplier - smoothWidth, 0.5, d);

    gl_FragColor = vec4(circleBorderColor.xyz, borderDepth);
} 
