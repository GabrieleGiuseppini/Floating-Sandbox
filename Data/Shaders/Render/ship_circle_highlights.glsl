###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inHighlight1; // Position, VertexSpacePosition
in vec4 inHighlight2; // Color, Progress
in float inHighlight3; // PlaneId

// Outputs
out vec2 vertexSpacePosition;
out vec3 color;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexSpacePosition = inHighlight1.zw;
    color = inHighlight2.xyz;

    gl_Position = paramOrthoMatrix * vec4(inHighlight1.xy, inHighlight3, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexSpacePosition; // [(-1.0, -1.0), (1.0, 1.0)]
in vec3 color;

void main()
{
    float d = distance(vertexSpacePosition, vec2(.0, .0));
    float alpha = smoothstep(0.68, 0.8, d) - smoothstep(0.8, 0.92, d);
        
    gl_FragColor = vec4(color, alpha);
}
