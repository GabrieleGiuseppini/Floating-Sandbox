###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inCrossOfLight1;  // Position, CenterPosition
in float inCrossOfLight2; // Progress

// Outputs
out vec2 vertexCenterPosition;
out float vertexProgress;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexCenterPosition = 
        (paramOrthoMatrix * vec4(inCrossOfLight1.zw, -1.0, 1.0)).xy;

    vertexProgress = inCrossOfLight2;

    gl_Position = paramOrthoMatrix * vec4(inCrossOfLight1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexCenterPosition;
in float vertexProgress; // [0.0, 1.0]

// Parameters
uniform vec2 paramViewportSize;

void main()
{
   // Map vertexProgress to -0.48 -> 0.48
    float progress = -0.48 + vertexProgress/(2. * .48); // -0.48 -> 0.48

    // No rotation for the time being
    // float angle = progress;

    // Calculate fragment's coordinates in the NDC space
    vec2 ndc = (gl_FragCoord.xy / paramViewportSize.xy) * 2.0 - vec2(1.0);

    // Center and flip vertically
    ndc = vec2(ndc.x - vertexCenterPosition.x, vertexCenterPosition.y - ndc.y);

    // ------------------    
    
    // No rotation for the time being
    //
    // mat2 rotationMatrix = mat2(
    //    cos(angle), -sin(angle),
    //    sin(angle), cos(angle));
    //
    // vec2 rotNdc = rotationMatrix * ndc;
    vec2 rotNdc = ndc;

    progress = pow(1. - abs(progress), 9.0);
    
    float alpha = 1.0 - smoothstep(0.0, progress, sqrt(abs(rotNdc.x * rotNdc.y)));

    gl_FragColor = vec4(1.0, 1.0, 1.0, alpha);
}
