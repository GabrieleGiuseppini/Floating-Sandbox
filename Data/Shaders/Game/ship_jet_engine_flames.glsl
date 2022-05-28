###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inJetEngineFlame1; // Position (vec2), PlaneId, PersonalitySeed
in vec2 inJetEngineFlame2; // FlameSpacePosition

// Outputs
out vec2 flameSpacePosition;
out vec2 noiseOffset;

// Params
uniform mat4 paramOrthoMatrix;
uniform float paramFlameProgress;

void main()
{
    flameSpacePosition = inJetEngineFlame2.xy;
    noiseOffset = vec2(inJetEngineFlame1.w, inJetEngineFlame1.w - paramFlameProgress);

    gl_Position = paramOrthoMatrix * vec4(inJetEngineFlame1.xyz, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 flameSpacePosition; // (x=[-1.0, 1.0], y=[0.0, 1.0])
in vec2 noiseOffset;

// The textures
uniform sampler2D paramNoiseTexture1;
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

// Params
uniform float paramFlameProgress;

//
// Loosely based on "Flame in the Wind" by kuvkar (https://www.shadertoy.com/view/4tXXRn)
//

float GetNoise(vec2 uv) // -> (-0.375, 0.375)
{
    float n = (texture2D(paramNoiseTexture1, uv).r - 0.5) * 0.5; // -0.25, 0.25
    n += (texture2D(paramNoiseTexture1, uv * 2.0).r - 0.5) * 0.5 * 0.5; // -0.375, 0.375
    
    return n;
}

mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);

    return m;
}

// -----------------------------------------------
void main()
{   
    // Fragments with alpha lower than this are discarded
    #define MinAlpha 0.2

    vec2 uv = flameSpacePosition;
    uv.x /= 2.0; // (x=[-0.5, 0.5], y=[0.0, 1.0])
    
    //
    // Flame time
    //
    
    #define FlameSpeed 0.6
    float flameTime = paramFlameProgress * FlameSpeed;
        
    //
    // Get noise for this fragment and time
    //
    
    #define NoiseResolution 0.4
    // (-0.375, 0.375)
    float fragmentNoise = GetNoise(uv * NoiseResolution + noiseOffset);
        
    //
    // Rotate fragment based on noise and vertical extent
    //
    
    float angle = fragmentNoise;

    // Magnify rotation amount based on distance from bottom of quad
    angle /= max(0.2, length(uv));

    // Straighten the flame at the bottom and make full turbulence higher up
    angle *= smoothstep(-0.1, 0.3, flameSpacePosition.y);

    // Smooth the angle
    angle *= 0.45;

    // Rotate!
    uv += GetRotationMatrix(angle) * uv;

    //
    // Calculate flameness
    //
    
    /*
    // Flame width
    float flameWidth = sqrt(flameSpacePosition.y); // Taper down 
    flameWidth *= 1.0 - smoothstep(0.7, 1.0, flameSpacePosition.y); // Taper up
    
    // Calculate flameness
    float flameness = 1.0 - smoothstep(0.0, flameWidth, abs(uv.x));
    */
    
    float flameWidth = min(1.0, sqrt(flameSpacePosition.y / 0.4)); // Taper down 
    float flameness = 1.0 - smoothstep(0.0, flameWidth, abs(uv.x));
    
    // Taper flame up depending on randomized height
    float variationH = (fragmentNoise + 0.5) * 1.4;
    flameness *= smoothstep(1.2, variationH * 0.5, flameSpacePosition.y);
    
    // Focus (less halo)
    #define FlameFocus 2.0
    flameness = pow(clamp(flameness, 0.0, 3.0), FlameFocus);

    
    //
    // Emit
    //
    
    vec3 col1 = mix(vec3(1.0, 1.0, 0.6), vec3(1.0, 1.0, 1.0), flameness * 0.9);
    col1 = mix(vec3(1.0, 0.4, 0.1), col1, smoothstep(0.3, 0.8, flameness));    

    // Blend with background
    float alpha = smoothstep(0.0, 0.5, flameness);
    gl_FragColor = mix(vec4(0.0), vec4(col1, 1.0), alpha);
}