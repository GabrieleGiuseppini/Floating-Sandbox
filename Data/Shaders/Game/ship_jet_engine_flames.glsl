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

    #define FlameSpeed 1.2
    noiseOffset = vec2(inJetEngineFlame1.w, inJetEngineFlame1.w - paramFlameProgress * FlameSpeed);

    gl_Position = paramOrthoMatrix * vec4(inJetEngineFlame1.xyz, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 flameSpacePosition; // (x=[-1.0, 1.0], y=[0.0, 1.0])
in vec2 noiseOffset;

// The textures
uniform sampler2D paramNoiseTexture;
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

//
// Loosely based on "Flame in the Wind" by kuvkar (https://www.shadertoy.com/view/4tXXRn)
//

float GetNoise(vec2 uv) // -> (-0.375, 0.375)
{
    float n = (texture2D(paramNoiseTexture, uv).r - 0.5) * 0.5; // -0.25, 0.25
    n += (texture2D(paramNoiseTexture, uv * 2.0).r - 0.5) * 0.5 * 0.5; // -0.375, 0.375
    
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

    vec2 uv = flameSpacePosition; // (x=[-1.0, 1.0], y=[0.0, 1.0])
        
    //
    // Get noise for this fragment and time
    //
    
    #define NoiseResolution 0.9
    // (-0.375, 0.375)
    float fragmentNoise = GetNoise(uv * NoiseResolution + noiseOffset);
        
    //
    // Rotate fragment based on noise
    //
    
    float angle = fragmentNoise;

    // Tune amount of chaos
    //angle *= 0.45;

    // Rotate (and add)
    uv += GetRotationMatrix(angle) * uv;

    //
    // Calculate flameness
    //
    
    float flameWidth = 0.1 + 0.4 * min(1.0, sqrt(flameSpacePosition.y / 0.4)); // Taper down  
    float flameness = 1.0 - abs(uv.x) / flameWidth;
    
    // Taper flame up depending on randomized height
    float variationH = (fragmentNoise + 0.5) * 1.4;
    flameness *= smoothstep(1.1, variationH * 0.5, flameSpacePosition.y);    
    
    //
    // Emit
    //
    
    vec3 col1 = mix(vec3(1.0, 1.0, 0.6), vec3(1.0, 1.0, 1.0), flameness);
    col1 = mix(vec3(227.0/255.0, 69.0/255.0, 11.0/255.0), col1, smoothstep(0.3, 0.8, flameness));    
    float alpha = smoothstep(0.0, 0.5, flameness);
    
    gl_FragColor = vec4(col1, alpha);
}