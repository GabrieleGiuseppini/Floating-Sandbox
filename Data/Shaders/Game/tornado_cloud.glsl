###VERTEX-120

#define in attribute
#define out varying

//
// Re-using Clouds' VBO
//
// Clouds' inputs are directly in NDC coordinates
//

// Inputs
in vec4 inTornadoCloud1; // Position (NDC) (vec2), TextureCoordinates (vec2)
in vec4 inTornadoCloud2; // VirtualTextureCoordinates (vec2), Darkening, Alpha
in vec2 inTornadoCloud3; // VolumetricGrowthProgress, TextureWidthAdjust (widest dimension)

// Outputs
out vec2 vertexTextureCoords;
out vec2 vertexVirtualTextureCoords;
out float vertexDarkness;
out float vertexAlpha;
out float vertexVolumetricGrowthProgress;
out float vertexTextureWidthAdjust;

void main()
{
    vertexTextureCoords = inTornadoCloud1.zw;
    vertexVirtualTextureCoords = inTornadoCloud2.xy;
    vertexDarkness = inTornadoCloud2.z;
    vertexAlpha = inTornadoCloud2.w;
    vertexVolumetricGrowthProgress = inTornadoCloud3.x;
    vertexTextureWidthAdjust = inTornadoCloud3.y;

    gl_Position = vec4(inTornadoCloud1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

#include "common.glslinc"
#include "lamp_tool.glslinc"

// Inputs from previous shader
in vec2 vertexTextureCoords;
in vec2 vertexVirtualTextureCoords; // [-1, +1]
in float vertexDarkness;
in float vertexAlpha;
in float vertexVolumetricGrowthProgress;
in float vertexTextureWidthAdjust;

// The texture
uniform sampler2D paramCloudsAtlasTexture;
uniform sampler2D paramNoiseTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;

void main()
{
    // Calculate lamp tool intensity
    float lampToolIntensity = CalculateLampToolIntensity(gl_FragCoord.xy);

    // Virtual texture coords -> Polar coords
    // (r, a) (r=[0.0, 1.0], a=[0.0, 2PI CCW from W])        
    vec2 ra = vec2(
        length(vertexVirtualTextureCoords), 
        atan(vertexVirtualTextureCoords.y, vertexVirtualTextureCoords.x));

    // Evolve with time, and use as noise sample coords
    #define SPEED 0.02
    #define DENSITY 6.0
    vec2 noiseCoords = vec2(ra.x / DENSITY - vertexVolumetricGrowthProgress * SPEED, ra.y / (2. * PI));
    float n = texture2D(paramNoiseTexture, noiseCoords).x; // [0.0, 1.0]    
    
    // Scale noise: [-amplitude/2, +amplitude/2]
    //
    // Amplitude is zero at borders
    #define BASE_AMPLITUDE 0.13 // Good for a texture of width 1.0
    float amplitude = 
        (BASE_AMPLITUDE * vertexTextureWidthAdjust)
        * (1.0 - smoothstep(0.85, 1.0, max(abs(vertexVirtualTextureCoords.x), abs(vertexVirtualTextureCoords.y))));
    n = (n - 0.5) * amplitude;
    
    // Zero at center
    n = n * smoothstep(0.2, 0.3, ra.x);
    
    // Sample cloud texture using noise as offset
    vec4 cloudColor = texture2D(paramCloudsAtlasTexture, vertexTextureCoords + vec2(n));

    // Merge with "smoke"
    //vec2 smokeCoords = (vertexVirtualTextureCoords + vec2(n)) * vec2(3., .5) * 1.2;
    vec2 smokeCoords = (vertexVirtualTextureCoords) * vec2(3., .4) * 1.2;
    float smokeSample = texture2D(paramNoiseTexture, smokeCoords).x; // [0.0, 1.0]    
    //cloudColor.rgb = mix(cloudColor.rgb * vertexDarkness, vec3(smokeSample), smokeSample * .2);
    //cloudColor.rgb = vec3(smokeSample * 0.8);


    float distanceFromBorder = 
        1.0 - smoothstep(0.5, 0.85, max(abs(vertexVirtualTextureCoords.x), abs(vertexVirtualTextureCoords.y)));
    //cloudColor.rgb = mix(cloudColor.rgb * vertexDarkness, vec3(smokeSample), smokeSample * .1 * distanceFromBorder);
    cloudColor.rgb = mix(cloudColor.rgb * vertexDarkness, vec3(smokeSample), smokeSample * distanceFromBorder);




    // Adjust for ambient light and mooncolor and (storm) darkness
    gl_FragColor = vec4(
        ApplyAmbientLight(
            cloudColor.rgb,
            paramEffectiveMoonlightColor,
            paramEffectiveAmbientLightIntensity,
            //lampToolIntensity) * vertexDarkness,
            lampToolIntensity),
        cloudColor.a * vertexAlpha);
} 
