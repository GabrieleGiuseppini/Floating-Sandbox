###VERTEX-120

#define in attribute
#define out varying

//
// Sky's inputs are directly in NDC coordinates
//

// Inputs
in vec2 inSky; // Position (NDC)

// Outputs
out float ndcY;

void main()
{
    ndcY = inSky.y;

    gl_Position = vec4(inSky, -1.0, 1.0); // Also clear depth buffer
}

###FRAGMENT-120

#include "static_parameters.glslinc"

#define in varying

// Inputs from previous shader
in float ndcY;

// Parameters
uniform vec3 paramCrepuscularColor;
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform vec3 paramFlatSkyColor;
uniform sampler2D paramNoiseTexture;

void main()
{
    //
    // The upper quarter interpolates between flat color and dark, based on ambient light;
    // The lower three quarters interpolate from flat color to crepuscular color to moonlight, based on ambient light.
    // 

    vec3 upperColor = paramFlatSkyColor * paramEffectiveAmbientLightIntensity;

    // AL between 0.5 and 1.0: interpolate between crepuscolar and flat, both darkened
    // AL between 0.0 and 0.5: interpolate between moonlight and above (crepuscolar)
    vec3 lowerColor = mix(paramCrepuscularColor, paramFlatSkyColor, max((paramEffectiveAmbientLightIntensity - 0.5) * (1/0.5), 0.0));
    lowerColor = mix(paramEffectiveMoonlightColor * 0.6, lowerColor * paramEffectiveAmbientLightIntensity, min(paramEffectiveAmbientLightIntensity * (1/0.5), 1.0));

    // Combine
    vec3 color = mix(lowerColor, upperColor, max(ndcY - 0.4, 0.0) * (1.0/0.6));

    // Dither
    vec2 noiseCoords = gl_FragCoord.xy / vec2(NOISE_2_WIDTH, NOISE_2_HEIGHT);
    float noise = texture2D(paramNoiseTexture, noiseCoords).x;
    color += mix(-0.5/255.0, 0.5/255.0, noise);
       
    // Output color
    gl_FragColor = vec4(color, 1.0);
} 
