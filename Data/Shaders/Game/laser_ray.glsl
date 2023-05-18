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
uniform sampler2D paramNoiseTexture;

// Params
uniform float paramTime;

float getNoise(float x, float block) // -1.0, 1.0
{
    float n = (texture2D(paramNoiseTexture, vec2(x, block)).r - 0.5) * 2.0;
    return n;
}

mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);

    return m;
}

void main()
{
    //
    // Narrow field (ray core)
    //
    
    vec2 nuv = raySpacePosition;
    
    float pixelNoiseN = getNoise(nuv.y / 4.0 - paramTime, 0.2);

    float widthModifierN = pixelNoiseN * 0.05;
    float xDistanceN = abs(nuv.x) + abs(widthModifierN);

    float innerThickness = 0.5 + (strength - 1.0) * 0.4; // s=1 => 0.5; s=2 => 0.9
    float alphaN = 1.0 - clamp(xDistanceN / innerThickness, 0.0, 1.0);    
    // Taper at end
    //alphaN *= 1.0 - clamp((nuv.y - 0.7) / 0.3, 0.0, 1.0);
    // Focus
    alphaN = alphaN * alphaN;


    //
    // Wide field (scatter halo)
    //
    
    vec2 wuv = raySpacePosition;
    
    float pixelNoiseW = getNoise(
        wuv.y / 4.0  - paramTime * 0.00,                                        // evolution along y
        (wuv.x - sign(wuv.x) * paramTime * 2.0 + pixelNoiseN * 0.08) * 0.2);    // evolution along x
    
    // Rotate pixel
    float angle = pixelNoiseW * 0.06;
    vec2 rotationOrigin = vec2(0.0, 10.0); // The further the rotation center, the more scattered the halo is
    vec2 displacedWuv = 
        GetRotationMatrix(angle) * (wuv - rotationOrigin)
        + rotationOrigin;
    
    float xDistanceW = abs(displacedWuv.x);

    float alphaW = 1.0 - clamp(xDistanceW, 0.0, 1.0);
    // Taper at end
    //alphaW *= 1.0 - clamp((wuv.y - 0.7) / 0.3, 0.0, 1.0);
    // Focus
    alphaW = alphaW * alphaW;
    
    

    //
    // Combine
    //
    
    // Leave early outside of ray
    //if (alphaW < 0.01)
    //    discard;
    
    vec3 colW = vec3(0.660, 0.0198, 0.0198);
    vec3 colN = vec3(0.988, 0.990, 0.842);
    
    vec3 col = mix(
        colW,
        colN,
        alphaN);
    
    //
    // Emit
    //

    gl_FragColor = vec4(col, min(1.0, alphaN + alphaW));
} 
