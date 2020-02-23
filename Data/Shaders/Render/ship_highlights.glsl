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
    #define BandWidth 0.3
        
    float d = distance(vertexSpacePosition, vec2(.0, .0));
    
    float t = fract((d - progress) * NBands);
    float t2 = 
        smoothstep(1.0 - BandWidth, 1.0, t)
        + smoothstep(1.0 - BandWidth, 1.0, 1.0 - t);
    
    t2 *= 1.0 - smoothstep(0.7, 1.0, d);
        
    gl_FragColor = vec4(color, t2);
}
