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

#define in varying

// Inputs from previous shader
in float ndcY;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramCrepuscularColor;
uniform vec3 paramFlatSkyColor;
uniform vec3 paramMoonlightColor;

void main()
{
    //
    // The upper quarter interpolates between flat color and dark, based on ambient light;
    // The lower three quarters interpolate from flat color to crepuscular color to moonlight, based on ambient light.
    // 

    vec3 upperColor = paramFlatSkyColor * paramEffectiveAmbientLightIntensity;

    // TODOHERE
    vec3 lowerColor = mix(paramMoonlightColor, paramCrepuscularColor, paramEffectiveAmbientLightIntensity);

    // Combine
    vec3 color = mix(lowerColor, upperColor, max(ndcY - 0.5, 0.0) * 2.0);
       
    // Output color
    gl_FragColor = vec4(color, 1.0);
} 
