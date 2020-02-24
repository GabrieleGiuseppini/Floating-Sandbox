###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inHighlight1; // Position, VertexSpacePosition
in vec4 inHighlight2; // Color, Progress

// Outputs
out vec2 vertexSpacePosition;
out vec3 color;
out float progress;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexSpacePosition = inHighlight1.zw;
    color = inHighlight2.xyz;
    progress = inHighlight2.w;

    gl_Position = paramOrthoMatrix * vec4(inHighlight1.xy, 0.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexSpacePosition; // [(-1.0, -1.0), (1.0, 1.0)]
in vec3 color;
in float progress; // [0.0, 1.0]

void main()
{
    #define NBands 4.0
    #define HalfBandWidth 0.4
        
    float d = distance(vertexSpacePosition, vec2(.0, .0));
    float d2 =d / 2.;
    
    float t = fract((d2 - progress) * NBands);
    float t2 = 
        smoothstep(1.0 - HalfBandWidth, 1.0, t)
        + smoothstep(1.0 - HalfBandWidth, 1.0, 1.0 - t);
    
    // Truncate
    t2 *= 1.0 - step(progress + HalfBandWidth / NBands + .05, d2); // truncate if d2 > outermost
    t2 *= step(progress - HalfBandWidth / NBands - 0.5 - .05, d2); // truncate if d2 < innermost
    
    // Fade out outside quad 
    t2 *= 1.0 - smoothstep(0.75, 1.0, d);
        
    gl_FragColor = vec4(color, t2);
}
