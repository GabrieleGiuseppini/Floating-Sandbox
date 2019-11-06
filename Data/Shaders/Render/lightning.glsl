###VERTEX

#version 120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inLightning1; // Position (vec2), SpacePosition (vec2)
in vec3 inLightning2; // BottomY, Progress, PersonalitySeed

// Outputs
out vec2 spacePosition;
out float bottomY;
out float progress;
out float personalitySeed;

void main()
{
    spacePosition = inLightning1.zw;
    bottomY = inLightning2.x;
    progress = inLightning2.y;
    personalitySeed = inLightning2.z;

    gl_Position = vec4(inLightning1.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

// Inputs
in vec2 spacePosition;
in float bottomY;
in float progress;
in float personalitySeed;

// Textures
uniform sampler2D paramNoiseTexture2;

float GetNoise(float v) // -> (-1.0, 1.0)
{
    return (texture2D(paramNoiseTexture2, vec2(.2, v)).r - .5) * 2.;
}

void main()
{
    #define ZigZagRate 8.
    #define ZigZagDensity1 .105   
    #define ZigZagDensity2 .65
    
    float randomSeed = personalitySeed * 77.7;
            
    // Get noise (2 octaves) for this fragment's y and time
    float variableNoiseOffset = floor(progress * ZigZagRate) / .567 + randomSeed;
    float fragmentNoise1 = GetNoise(spacePosition.y * ZigZagDensity1 + variableNoiseOffset);
    float fragmentNoise2 = GetNoise(spacePosition.y * ZigZagDensity2 + variableNoiseOffset);
        
    // Calculate deviation based on noise
    float deviation = 
        fragmentNoise1 * .08
        + fragmentNoise2 * .08 * .2;
    
    // Calculate the bottom Y of the lightning at this moment:
    // - Low progress: close to the top, i.e. 1.
    // - High progress: close the desired bottom Y
    float variableBottomY = 1. + (bottomY - 1.) * smoothstep(-.1, .3, progress);
    
    // Taper deviation up and down    
    deviation *= smoothstep(variableBottomY, variableBottomY + .2, spacePosition.y); // Bottom
    deviation *= smoothstep(0., .05, 1. - spacePosition.y); // Up
                
    // Calculate thickness
    #define MaxHalo -6.
    #define MinHalo -100.
    float k = MaxHalo + (MinHalo - MaxHalo) * (spacePosition.y - bottomY) / (1. - bottomY);
    float thickness = exp(k * abs(spacePosition.x + deviation)); // Deviate laterally
    
    // Make pointy at bottom
    thickness *= smoothstep(variableBottomY, variableBottomY + .2, spacePosition.y);
        
    
    //
    // Emit
    //
    
    vec3 col1 = mix(vec3(0.0, 0.0, 0.0), vec3(0.6, 0.8, 1.0), thickness);
    gl_FragColor = vec4(col1, thickness);
} 
