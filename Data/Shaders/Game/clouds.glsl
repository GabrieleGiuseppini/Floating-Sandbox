###VERTEX-120

#define in attribute
#define out varying

//
// Clouds' inputs are directly in NDC coordinates
//

// Inputs
in vec4 inCloud1; // Position (NDC) (vec2), TextureCoordinates (vec2)
in vec4 inCloud2; // TextureCenterCoordinates (vec2), VirtualTextureCoordinates (vec2) 
in vec2 inCloud3; // Darkening, VolumetricGrowthProgress

// Outputs
out vec2 texturePos;
out vec2 textureCenterPos;
out vec2 virtualTexturePos;
out float darkness;
out float volumetricGrowthProgress;

void main()
{
    texturePos = inCloud1.zw;
    textureCenterPos = inCloud2.xy;
    virtualTexturePos = inCloud2.zw;
    darkness = inCloud3.x;
    volumetricGrowthProgress = inCloud3.y;

    gl_Position = vec4(inCloud1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 texturePos;
in vec2 textureCenterPos;
in vec2 virtualTexturePos;
in float darkness;
in float volumetricGrowthProgress;

// The texture
uniform sampler2D paramCloudsAtlasTexture;
uniform sampler2D paramNoiseTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;

void main()
{
    // Sample texture
    vec4 textureColor = texture2D(paramCloudsAtlasTexture, texturePos);

    // Adjust for ambient light and mooncolor and (storm) darkness
    vec3 cloudColor = textureColor.xyz * mix(paramEffectiveMoonlightColor, vec3(1.), paramEffectiveAmbientLightIntensity) * darkness;

    //
    // Apply noise as growth from center
    //

    // Center uv ~[-1., +1.]
    vec2 virtualTextureCoordsCentered = (virtualTexturePos - vec2(0.5, 0.5)) * 2.;

    // Convert to polar coords
    // (r, a) (r=[0.0, 1.0], a=[0.0, 1.0 CCW from W])
    #define PI 3.1415926
    vec2 ra = vec2(
        length(virtualTextureCoordsCentered), 
        atan(virtualTextureCoordsCentered.y, virtualTextureCoordsCentered.x));
    // Add growth and magnify to taste
    #define GROWTH_SPEED 0.15
    #define MAG 6.0
    vec2 ra2 = vec2((ra.x - volumetricGrowthProgress * GROWTH_SPEED) / MAG, ra.y / (2. * PI));
    float n = texture2D(paramNoiseTexture, ra2).x;

    // Silence towards center
    float a = 1.0 - abs(n) * smoothstep(0.2, 0.4, ra.x); 
    // Modulate to desired incisiveness, and focus constrast to lower range
    #define VAR 0.28
    a = (1.0 - VAR) + VAR * smoothstep(0.0, 0.8, a);
    
    // Combine into final color
    gl_FragColor = vec4(
        cloudColor,
        textureColor.a * a);
} 
