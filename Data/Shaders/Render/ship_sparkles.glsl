###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inSparkle1; // Position, PlaneId, Progress
in vec4 inSparkle2; // VelocityVector, SparkleSpacePosition

// Outputs
out float progress;
out vec2 velocityVector;
out vec2 sparkleSpacePosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    progress = inSparkle1.w;
    velocityVector = inSparkle2.xy;
    sparkleSpacePosition = inSparkle2.zw;

    gl_Position = paramOrthoMatrix * vec4(inSparkle1.xyz, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in float progress; // [0.0, 1.0]
in vec2 velocityVector; // [(-1.0, -1.0), (1.0, 1.0)]
in vec2 sparkleSpacePosition; // [(-1.0, -1.0), (1.0, 1.0)]

vec3 GetHeatColor(float h) // 0.0 -> 1.0
{  
    vec3 whiteTint = mix(
        vec3(1.0, 1.0, 0.19), 	// yellow
        vec3(1.0, 1.0, 1.0),	// white
        smoothstep(0.84, 1.0, h));
    
    return whiteTint;
}    

void main()
{
    vec2 velocityVectorAdj = velocityVector / 2.0;     
    
    // v = (0.0, 0.0)
    // w = -velocity
	// Consider the line extending the segment, parameterized as v + t (w - v).
  	// We find projection of point p onto the line. 
  	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
  	// We clamp t from [0,1] to handle points outside the segment vw.
    float l2 = (velocityVectorAdj.x * velocityVectorAdj.x + velocityVectorAdj.y * velocityVectorAdj.y);  // i.e. |w-v|^2 -  avoid a sqrt
  	float t = max(0.0, min(1.0, dot(sparkleSpacePosition, velocityVectorAdj) / l2));
    vec2 projection = t * velocityVectorAdj;  // Projection falls on the segment
    float vectorDistance = length(projection - sparkleSpacePosition);
    
    float centerDistance = length(sparkleSpacePosition);
    
    float vectorDistanceNormalized = smoothstep(0.0, 0.15, vectorDistance);
    float centerDistanceNormalized = smoothstep(0.0, 0.20, centerDistance);
    
    float heat = 1.0 - (vectorDistanceNormalized * centerDistanceNormalized);
    
    // Colorize
    vec3 col = GetHeatColor(heat);
    float alpha = 1.0 - smoothstep(0.2, 0.35, 1.0 - heat);

    alpha *= 1.0 - progress;

    gl_FragColor = vec4(col, alpha);
}
