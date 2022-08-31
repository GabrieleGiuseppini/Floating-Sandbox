###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inSparkle1; // Position, PlaneId, Progress
in vec2 inSparkle2; // SparkleSpacePosition

// Outputs
out float progress;
out vec2 sparkleSpacePosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    progress = inSparkle1.w;
    sparkleSpacePosition = inSparkle2.xy;

    gl_Position = paramOrthoMatrix * vec4(inSparkle1.xyz, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float progress; // [0.0, 1.0]
in vec2 sparkleSpacePosition; // x: from -1.0 (left) to +1.0 (right); y: from -1.0 (top) to +1.0 (bottom)

void main()
{
    #define HeadRadius 0.05
    
    float yc = sparkleSpacePosition.y - (-1.0 + HeadRadius);  // -HeadRadius, 0.0, 2.0 - HeadRadius

    float ycBottomScaled = yc / (2.0 - HeadRadius); // Bottom 0.0: 0.0 ... 1.0
    
    float yp = 
        step(yc, 0.0) * yc / HeadRadius
        + step(0.0, yc) * (ycBottomScaled * ycBottomScaled);
        
    float antiProgress = 1.0 - progress;
    float depth = max(
        (antiProgress - length(vec2(sparkleSpacePosition.x, yp))) / antiProgress,
        0.0);
    
    float alpha = depth;
    
    // Focus
    alpha = alpha * alpha;

    // Leave early outside of sparkle
    if (alpha < 0.01)
        discard;
    
    // Blend between yellow/white and orange/red depending on time
    vec3 col = mix(
        vec3(1.0, 1.0, 0.90),	     // yellow/white
        vec3(0.320, 0.0896, 0.0128), // orange/red
        progress);

    // -----------------------------------

    gl_FragColor = vec4(col, alpha);
}
