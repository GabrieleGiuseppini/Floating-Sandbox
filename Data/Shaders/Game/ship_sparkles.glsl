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
    // TODO: take as param, depending on length and aspect ratio    
    #define HeadRadius 0.3
    
    float yc = sparkleSpacePosition.y - (-1.0 + HeadRadius);
    
    float yp = 
        step(yc, 0.0) * yc / HeadRadius
        + step(0.0, yc) * yc / (2.0 - HeadRadius);
        
    float antiProgress = 1.0 - progress;
    float depth = max(
        (antiProgress - length(vec2(sparkleSpacePosition.x, yp))) / antiProgress,
        0.0);
    
    float alpha = depth;
    
    // Focus
    alpha = alpha * alpha;
    
    // progress = 0.0: whole sparkle is yellow/white
    // progress > 0.0: sparkle is orange at y=+1 end, yellow/white at y=-1 end
    float progressFactor = progress * (1.0 - (1.0 - yp) / 2.0);
    vec3 col = mix(
        vec3(1.0, 1.0, 0.90),	// yellow/white
        vec3(0.9, 0.50, 0.14), 	// orange 
        progressFactor);

    // -----------------------------------

    gl_FragColor = vec4(col, alpha);
}
