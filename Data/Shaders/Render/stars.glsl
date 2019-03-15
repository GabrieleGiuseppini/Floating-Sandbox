###VERTEX

#version 120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec3 inStar; // Position (vec2), Brightness (float)

// Outputs
out float vertexBrightness;

void main()
{
    vertexBrightness = inStar.z; 
    gl_Position = vec4(inStar.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

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
