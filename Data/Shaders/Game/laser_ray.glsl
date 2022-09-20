###VERTEX-120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inLaserRay1; // NdcPosition (vec2), SpacePosition (vec2)
in float inLaserRay2; // Strength (float)

// Outputs
out vec2 raySpacePosition;
out float strength;

void main()
{
    raySpacePosition = inLaserRay1.zw;
    strength = inLaserRay2;

    gl_Position = vec4(inLaserRay1.xy, -1.0, 1.0);
}


###FRAGMENT-120

#define in varying

// Inputs
in vec2 raySpacePosition;
in float strength;

// Textures
uniform sampler2D paramNoiseTexture2;

// Params
uniform float paramTime;

float getNoise(float x, float block) // -1.0, 1.0
{
    float n = (texture2D(paramNoiseTexture2, vec2(x, block)).r - 0.5) * 2.0;
    return n;
}

void main()
{
    // Width
    float widthModifier = getNoise(raySpacePosition.y / 10.0 - paramTime, 0.2) * 0.1;
    float xDistance = abs(raySpacePosition.x) + abs(widthModifier);
    
    // Wide alpha
    float alpha1 = 1.0 - clamp(xDistance, 0.0, 1.0);    
    // Taper at end
    alpha1 *= 1.0 - clamp((raySpacePosition.y - 0.7) / 0.3, 0.0, 1.0);
    // Focus
    alpha1 = alpha1 * alpha1;
    
    // Narrow alpha
    float alpha2 = 1.0 - clamp(xDistance / 0.5, 0.0, 1.0);    
    // Taper at end
    alpha2 *= 1.0 - clamp((raySpacePosition.y - 0.7) / 0.3, 0.0, 1.0);
    // Focus
    alpha2 = alpha2 * alpha2;
    
    // Leave early outside of ray
    if (alpha1 < 0.01)
        discard;
    
    vec3 colW = vec3(1.0, 0.4, 0.1);
    vec3 colN = vec3(0.988, 0.990, 0.842);
    
    vec3 col = mix(
        colW,
        colN,
        alpha2);
    
    //
    // Emit
    //

    gl_FragColor = vec4(col, alpha1);
} 
