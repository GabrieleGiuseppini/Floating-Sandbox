###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec3 inFlame1; // Position, PlaneId
in vec2 inFlame2; // FlameSpacePosition

// Outputs
out vec2 flameSpacePosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    flameSpacePosition = inFlame2.xy;

    gl_Position = paramOrthoMatrix * vec4(inFlame1.xyz, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 flameSpacePosition; // (x=[-0.5, 0.5], y=[0.0, 1.0])

// The texture
uniform sampler2D paramNoiseTexture;

// Params
uniform float paramTime;
uniform float paramWindSpeedMagnitude;

//
// Based on "Flame in the Wind" by kuvkar (https://www.shadertoy.com/view/4tXXRn)
//

float GetNoise(vec2 uv) // -> (0.25, 0.75)
{
    float n = (texture2D(paramNoiseTexture, uv).r - 0.5) * 0.5;
    n += (texture2D(paramNoiseTexture, uv * 2.0).r - 0.5) * 0.5 * 0.5;
    
    return n + 0.5;
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
    vec2 uv = flameSpacePosition - vec2(0.0, 0.5); // (x=[-0.5, 0.5], y=[-0.5, 0.5])

    // Flame time
    #define FlameSpeed 0.23
    float flameTime = paramTime * FlameSpeed;
    
    // Get noise for this fragment and time
    #define NoiseResolution 0.7
    float fragmentNoise = GetNoise(uv * NoiseResolution + vec2(0.0, -flameTime/0.8));

    //
    // Rotate fragment based on noise and vertical extent
    //
    
    float angle = (fragmentNoise - 0.5);

    // Magnify rotation amount based on distance from center of quad
    angle /= max(0.1, length(uv));

    // Straighten the flame at the bottom and make full turbulence higher up
    angle *= smoothstep(-0.1, 0.5, flameSpacePosition.y);

    // Smooth the angle
    angle *= 0.45;

    // Rotate!
    uv += GetRotationMatrix(angle) * uv;    

    
    //
    // Apply wind
    //

    // Rotation angle
    float windAngle = -sign(paramWindSpeedMagnitude) * 2.0 * smoothstep(0.0, 100.0, abs(paramWindSpeedMagnitude));
    
    // Randomize a bit
    windAngle *= 1.1 * fragmentNoise;

    // Rotation angle is higher the higher we go
    windAngle *= flameSpacePosition.y;
    
    // Rotate around bottom
    uv = GetRotationMatrix(windAngle) * (uv + vec2(0.0, 0.5)) - vec2(0.0, 0.5);


    //
    // Calculate thickness
    //
    
    #define FlameWidth 0.5
    float thickness = 1.0 - smoothstep(0.1, FlameWidth, abs(uv.x));
    
    // Taper flame depending on randomized height
    float variationH = fragmentNoise * 1.4;
    thickness *= smoothstep(1.3, variationH * 0.5, flameSpacePosition.y); // Taper up
    thickness *= smoothstep(-0.1, 0.15, flameSpacePosition.y); // Taper down
    
    // Focus (less halo, larger body)
    #define FlameFocus 2.0
    thickness = pow(clamp(thickness, 0.0, 3.0), FlameFocus);


    //
    // Emit
    //

    if (thickness < 0.1)
        discard;

    vec3 col1 = mix(vec3(1.0, 1.0, 0.6), vec3(1.0, 1.0, 1.0), thickness);
    col1 = mix(vec3(1.0, 0.2, 0.0), col1, smoothstep(0.3, 0.8, thickness));    

    gl_FragColor = vec4(col1, smoothstep(-0.1, 0.5, thickness));
} 
