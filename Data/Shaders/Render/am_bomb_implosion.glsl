###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inAMBombImplosion1;  // Position, CenterPosition
in float inAMBombImplosion2; // Progress

// Outputs
out vec2 vertexCenterPosition;
out float vertexProgress;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexCenterPosition = 
        (paramOrthoMatrix * vec4(inAMBombImplosion1.zw, -1.0, 1.0)).xy;

    vertexProgress = inAMBombImplosion2;

    gl_Position = paramOrthoMatrix * vec4(inAMBombImplosion1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexCenterPosition;
in float vertexProgress;

// Parameters
uniform vec2 paramViewportSize;

void main()
{
    float progress = vertexProgress - 0.5; // (-0.5, 0.5]

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
    
    progress = pow(abs(progress), 3.0);
        
    // Calculate tapering along each arm
    float taperX = pow(100000.0 * abs(rotNdc.x), 1.6) * progress;
    float taperY = pow(100000.0 * abs(rotNdc.y), 1.6) * progress;
    
    // Calculate width along arm
    float sx = max(0.0, (1.0 - rotNdc.x * rotNdc.x * taperY));
    float sy = max(0.0, (1.0 - rotNdc.y * rotNdc.y * taperX));
    float alpha = (sx + sy) / 2.0;

    gl_FragColor = vec4(1.0, 1.0, 1.0, alpha);
}
