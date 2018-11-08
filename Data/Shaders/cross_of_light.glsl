###VERTEX

#version 130

// Inputs
in vec4 inSharedAttribute0;  // Position, CenterPosition
in float inSharedAttribute1; // Progress

// Outputs
out vec2 vertexCenterPosition;
out float vertexProgress;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexCenterPosition = inSharedAttribute0.zw;
    vertexProgress = inSharedAttribute1;

    gl_Position = paramOrthoMatrix * vec4(inSharedAttribute0.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 vertexCenterPosition;
in float vertexProgress;

// Parameters
uniform vec2 viewportSize;

void main()
{
    float progress = vertexProgress - 0.5; // (-0.5, 0.5]
    float angle = 0.0; // TODO

    float minDimension = min(viewportSize.x, viewportSize.y);
    vec2 ndc = vec2(
        (gl_FragCoord.x - vertexCenterPosition.x), 
        (vertexCenterPosition.y - gl_FragCoord.y)) * 2.0 / minDimension;

    // ------------------    
    
    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));
    
    vec2 rotNdc = rotationMatrix * ndc;
    
    progress = pow(progress, 3.0);
        
    // Calculate tapering along each arm
    float taperX = pow(100000.0 * rotNdc.x, 1.6) * progress;
    float taperY = pow(100000.0 * rotNdc.y, 1.6) * progress;
    
    // Calculate width along arm
    float sx = max(0.0, (1.0-rotNdc.x * rotNdc.x * taperY));
    float sy = max(0.0, (1.0-rotNdc.y * rotNdc.y * taperX));
    float alpha = sx + sy;

    // TODO
    // gl_FragColor = vec4(1.0, 1.0, 1.0, alpha);
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
