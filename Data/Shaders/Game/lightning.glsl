###VERTEX-120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inLightning1; // Position (vec2), SpacePositionX, BottomY
in vec3 inLightning2; // Progress, RenderProgress, PersonalitySeed

// Outputs
out vec2 spacePosition;
out float bottomY;
out float progress;
out float renderProgress;
out float personalitySeed;

void main()
{
    spacePosition = vec2(inLightning1.z, inLightning1.y);
    bottomY = inLightning1.w;
    progress = inLightning2.x;
    renderProgress = inLightning2.y;
    personalitySeed = inLightning2.z;

    gl_Position = vec4(inLightning1.xy, -1.0, 1.0);
}


###FRAGMENT-120

#define in varying

// Inputs
in vec2 spacePosition;
in float bottomY;
in float progress;
in float renderProgress;
in float personalitySeed;

// Textures
uniform sampler2D paramNoiseTexture;

// Parameters
uniform float paramEffectiveAmbientLightIntensity;

float GetNoise(float v) // -> (-1.0, 1.0)
{
    return (texture2D(paramNoiseTexture, vec2(.2, v)).r - .5) * 2.;
}

void main()
{
    #define ZigZagDensity1 .4
    #define ZigZagAmplitude1 .25
    #define ZigZagDensity2 2.1
    #define ZigZagAmplitude2 .2 * .2
        
    float randomSeed = personalitySeed * 77.7;
            
    // Get noise (2 octaves) for this fragment's y and time
    float variableNoiseOffset = floor(progress * 8.) / .567 + randomSeed;
    float fragmentNoise1 = GetNoise(spacePosition.y * ZigZagDensity1 + variableNoiseOffset);
    float fragmentNoise2 = GetNoise(spacePosition.y * ZigZagDensity2 + variableNoiseOffset);
        
    // Calculate deviation based on noise
    float deviation = 
        fragmentNoise1 * ZigZagAmplitude1
        + fragmentNoise2 * ZigZagAmplitude2;
    
    // Calculate the bottom Y of the lightning at this moment:
    // - Low progress: close to the top, i.e. 1.
    // - High progress: close the desired bottom Y
    //   - Touches bottom at progress=.3
    float variableBottomY = 1. + (bottomY - 1.) * renderProgress;
    
    // Taper deviation up and down    
    deviation *= smoothstep(variableBottomY, variableBottomY + .2, spacePosition.y); // Bottom
    deviation *= smoothstep(0., .05, 1. - spacePosition.y); // Up
                
    // Calculate thickness
    #define MaxHalo -6.
    #define MinHalo -100.
    float k = MaxHalo + (MinHalo - MaxHalo) * (spacePosition.y - bottomY) / (1. - bottomY);
    float thickness = exp(k * abs(spacePosition.x + deviation)); // Deviate laterally
    
    // Make pointy at bottom
    thickness *= smoothstep(variableBottomY - .01, variableBottomY + .2, spacePosition.y);
        
    
    //
    // Emit
    //

    vec3 lightningColor = mix(vec3(1.), vec3(0.6, 0.8, 1.0), 1.0 - paramEffectiveAmbientLightIntensity);
    gl_FragColor = vec4(lightningColor, thickness);
} 
