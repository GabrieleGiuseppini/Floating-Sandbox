###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inCanvas; // Vertex position (ship space), Normalized coords (0->1)

// Outputs
out vec2 normalizedCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    normalizedCoords = inCanvas.zw;
    gl_Position = paramOrthoMatrix * vec4(inCanvas.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 normalizedCoords; // (x=[0.0, 1.0], y=[0.0, 1.0])

// Params
uniform vec2 paramPixelSize;
uniform vec3 paramCanvasBackgroundColor;

void main()
{
    #define SafetyMultiplier 1.25

    float borderDepthX = 
        step(normalizedCoords.x, paramPixelSize.x * SafetyMultiplier)
        + step(1.0 - paramPixelSize.x * SafetyMultiplier, normalizedCoords.x);

    float borderDepthY = 
        step(normalizedCoords.y, paramPixelSize.y * SafetyMultiplier)
        + step(1.0 - paramPixelSize.y * SafetyMultiplier, normalizedCoords.y);
        
    float borderDepth = min(borderDepthX + borderDepthY, 1.);

    gl_FragColor = mix(
        vec4(paramCanvasBackgroundColor, 1.),   // BG color
        vec4(.35, .35, .35, 1.),                // Border color      
        borderDepth);
} 
