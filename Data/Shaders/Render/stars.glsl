###VERTEX

#version 130

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec3 inSharedAttribute0; // Position (vec2), Brightness (float)

// Outputs
out float vertexBrightness;

void main()
{
    vertexBrightness = inSharedAttribute0.z; 
    gl_Position = vec4(inSharedAttribute0.xy, -1.0, 1.0);
}


###FRAGMENT

#version 130

// Inputs from previous shader
in float vertexBrightness;

// Parameters        
uniform float paramStarTransparency;

void main()
{
    gl_FragColor = vec4(
        vertexBrightness, 
        vertexBrightness, 
        vertexBrightness, 
        vertexBrightness * paramStarTransparency);
} 
