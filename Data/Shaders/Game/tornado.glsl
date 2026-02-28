###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inTornado1;  // Position, TornadoSpaceCoords
in vec4 inTornado2; //  BottomWidthFraction, StrengthMultiplier, HeatDepth, VisibilityAlpha

// Outputs
out vec2 vertexSpaceCoords;
out float vertexBottomWidthFraction;
out float vertexStrengthMultiplier;
out float vertexHeatDepth;
out float vertexVisibilityAlpha;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexSpaceCoords = inTornado1.zw;
    vertexBottomWidthFraction = inTornado2.x;
    vertexStrengthMultiplier = inTornado2.y;
    vertexHeatDepth = inTornado2.z;
    vertexVisibilityAlpha = inTornado2.w;

    gl_Position = paramOrthoMatrix * vec4(inTornado1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"

// Inputs from previous shader
in vec2 vertexSpaceCoords;
in float vertexBottomWidthFraction;
in float vertexStrengthMultiplier;
in float vertexHeatDepth;
in float vertexVisibilityAlpha;

// Textures
uniform sampler2D paramNoiseTexture;
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

// Params
uniform vec2 paramAtlasTile1LeftBottomTextureCoordinates; // Inclusive of dead-center
uniform vec2 paramAtlasTile1Size; // Inclusive of dead-center
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform float paramSimulationTime;

void main()
{
    //
    // Contorsions
    //
    
    float contorsionNoise = sin(vertexSpaceCoords.y * 15. -paramSimulationTime * 2.8);
    vec2 contortedVertexSpaceCoords = vec2(
        vertexSpaceCoords.x + 
            (contorsionNoise * 0.08)
            * smoothstep(0.0, 0.1, vertexSpaceCoords.y),
        vertexSpaceCoords.y);    

    //
    // Smoke
    //
    
    float width = vertexBottomWidthFraction + (1.0 - vertexBottomWidthFraction) * contortedVertexSpaceCoords.y;      
    
    // Noise
    
    //#define VORTEX_SPEED 3.9
    #define VORTEX_SPEED 0.9
    //#define SMOKE_NOISE_RESOLUTION_X 1.0
    //#define SMOKE_NOISE_RESOLUTION_Y 2.0
    #define SMOKE_NOISE_RESOLUTION_X 0.15 * 2.
    #define SMOKE_NOISE_RESOLUTION_Y 0.2 * 8.

    float nx = asin(contortedVertexSpaceCoords.x / width) * width;
    float nxt = paramSimulationTime * VORTEX_SPEED;
    
    vec2 smokeNoiseCoords = vec2(
        nx * SMOKE_NOISE_RESOLUTION_X + fract(nxt * SMOKE_NOISE_RESOLUTION_X),
        contortedVertexSpaceCoords.y * SMOKE_NOISE_RESOLUTION_Y);
    
    // Perlin 1024
    float smokeNoise = texture2D(paramNoiseTexture, smokeNoiseCoords).r;
       
    // Mask sides - darker noise restricting width
    float noiseWidthReduction = 0.3 * (1.0 - smokeNoise);
    // ...but avoid reducing too much at bottom
    noiseWidthReduction *= smoothstep(-0.1, 0.5, contortedVertexSpaceCoords.y);
    #define SMOOTH_W 0.1
    float alpha = 1.0 - smoothstep(
        max(width - noiseWidthReduction - SMOOTH_W, 0.), 
        max(width - noiseWidthReduction, 0.), 
        abs(contortedVertexSpaceCoords.x));

    // Top flange
    alpha *= 1.0 - smoothstep(0.92, 1.0, contortedVertexSpaceCoords.y + 0.05 * smokeNoise);

    // Bottom flange
    alpha *= smoothstep(0.0, 1.0-0.99, contortedVertexSpaceCoords.y - 0.05 * smokeNoise);
    
    // Color
    vec3 smokeColor = vec3(smokeNoise * 0.8); // On the darker side

    // Modulate with strength
    smokeColor *= 1. / vertexStrengthMultiplier;
    
    // Apply ambient lighting
    float lampToolIntensity = CalculateLampToolIntensity(gl_FragCoord.xy);    
    vec3 smokeColor2 = ApplyAmbientLight(
        smokeColor,
        paramEffectiveMoonlightColor,
        paramEffectiveAmbientLightIntensity,
        lampToolIntensity);
    
    // Actual whole alpha
    alpha = alpha * vertexVisibilityAlpha;
    // Darken with lower visibility - preventing white halo when alpha~=0
    smokeColor2 *= vertexVisibilityAlpha;

    //
    // Fire
    //
    
    #define FIRE_RESOLUTION_X 0.05
    #define FIRE_RESOLUTION_Y 1.0
    
    // 0..1
    vec2 fireCoords = vec2(
        nx * FIRE_RESOLUTION_X + fract(nxt * FIRE_RESOLUTION_X),
        contortedVertexSpaceCoords.y * FIRE_RESOLUTION_Y);   
                
    //
    // Rotate fragment based on noise and vertical extent
    //
    
    #define ROT_RESOLUTION 0.02
    vec2 rotationNoiseCoords = vec2(
        nx * FIRE_RESOLUTION_X * ROT_RESOLUTION + fract(nxt * FIRE_RESOLUTION_X * ROT_RESOLUTION),
        contortedVertexSpaceCoords.y * FIRE_RESOLUTION_Y * ROT_RESOLUTION -fract(paramSimulationTime * 0.0065));  
    
    float rotationNoise = texture2D(paramNoiseTexture, rotationNoiseCoords).r;
    
    float angle = (rotationNoise - 0.5);
        
    // Straighten the flame at the bottom and make full turbulence higher up
    angle *= smoothstep(-0.8, 0.5, contortedVertexSpaceCoords.y);    
    
    // Smooth the angle
    angle *= 0.25;
    
    // Rotate!
    fireCoords.x = fireCoords.x * cos(angle) - fireCoords.y * sin(angle);
    
    
    //
    // Sample fire
    //
    
    vec2 fireTextureCoords = paramAtlasTile1LeftBottomTextureCoordinates + paramAtlasTile1Size * fract(fireCoords);
    vec4 fireColor = texture2D(paramGenericLinearTexturesAtlasTexture, fireTextureCoords);
    
    
    //
    // Combine
    //
    
    // More fire where it's darker, with threshold moving with vertexHeatDepth
    float smokeDarkness = 1.0 - smokeNoise;
    float mask = smoothstep(1.0 - vertexHeatDepth, 1.0, smokeDarkness);
    // ...but go full vertexHeatDepth at bottom
    mask = mix(vertexHeatDepth, mask, contortedVertexSpaceCoords.y);
    // ...and modulate with smoke
    mask *= smokeColor.r;

    gl_FragColor = vec4(
        mix(smokeColor2, fireColor.rgb, mask * fireColor.a),
        alpha);
}
