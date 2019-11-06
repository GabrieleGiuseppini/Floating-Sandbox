###VERTEX

#version 120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inLightning1; // Position (vec2), SpacePosition (vec2)
in vec2 inLightning2; // BottomY, Progress, PersonalitySeed

// Outputs
out vec2 uv;

void main()
{
    uv = inLightning1.zw;
    gl_Position = vec4(inLightning1.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

// Inputs
in vec2 uv;

// Parameters        
uniform float paramTime;

void main()
{
    //
    // ---------------------------------------------
    //

    float t = 1. - abs(uv.x - .2) / .2;
    t += 1. - abs(uv.y - .2) / .2;

    t *= paramTime / paramTime;
    
    // Output to screen
    gl_FragColor = vec4(t, .0, .0, 1.);
} 
