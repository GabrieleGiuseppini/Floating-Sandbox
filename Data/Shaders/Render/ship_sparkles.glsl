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

void main()
{
    vec2 velocityVectorAdj = velocityVector / 1.0;
    
    // v = (0.0, 0.0)
    // w = -velocity
    // p = uw
	// Consider the line extending the segment, parameterized as v + t (w - v).
  	// We find projection of point p onto the line. 
  	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
  	// We clamp t from [0,1] to handle points outside the segment vw.
    float l2 = (velocityVectorAdj.x * velocityVectorAdj.x + velocityVectorAdj.y * velocityVectorAdj.y);  // i.e. |w-v|^2 -  avoid a sqrt
    float t = dot(sparkleSpacePosition, velocityVectorAdj) / l2;
    t = max(-1.0, min(1.0, t)); // [-1.0, 1.0]
    vec2 projection = t * velocityVectorAdj;    
    float vectorDistance = length(projection - sparkleSpacePosition);
    
    // d = Density: 1.0 at center, 0.0 at border
    #define LineThickness 0.1
    float d = 1.0 - vectorDistance/LineThickness;
    
    if (d < 0.01)
        discard;
    
    vec3 col = mix(
        vec3(1.0, 1.0, 0.80),	// yellow/white
        vec3(0.8, 0.50, 0.14), 	// orange        
        smoothstep(0.0, 0.35, progress) * (1.0 - (t + 1.0) / 2.0));

    float alpha = d;

    alpha *= 1.0 - progress;

    gl_FragColor = vec4(col, alpha);
}
