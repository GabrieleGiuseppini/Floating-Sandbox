###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inRectOverlay1; // Vertex position (ship space), Normalized coords (0->1)
in vec3 inRectOverlay2; // Color

// Outputs
out vec2 normalizedCoords;
out vec3 rectBorderColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    normalizedCoords = inRectOverlay1.zw;
    rectBorderColor = inRectOverlay2;
    gl_Position = paramOrthoMatrix * vec4(inRectOverlay1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 normalizedCoords; // (x=[0.0, 1.0], y=[0.0, 1.0])
in vec3 rectBorderColor;

// Params
uniform vec2 paramPixelSize;

void main()
{
    #define SafetyMultiplier 1.25

    float borderDepthX = 
        1.0 - smoothstep(0.0, paramPixelSize.x * SafetyMultiplier, normalizedCoords.x)
        + smoothstep(1.0 - paramPixelSize.x * SafetyMultiplier, 1.0, normalizedCoords.x);

    float borderDepthY = 
        1.0 - smoothstep(0.0, paramPixelSize.y * SafetyMultiplier, normalizedCoords.y)
        + smoothstep(1.0 - paramPixelSize.y * SafetyMultiplier, 1.0, normalizedCoords.y);
        
    float borderDepth = min(borderDepthX + borderDepthY, 1.);

    gl_FragColor = vec4(rectBorderColor.xyz, borderDepth);
} 
