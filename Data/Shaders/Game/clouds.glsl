###VERTEX-120

#define in attribute
#define out varying

//
// Clouds' inputs are directly in NDC coordinates
//

// Inputs
in vec4 inCloud1; // Position (NDC) (vec2), TextureCoordinates (vec2)
in vec4 inCloud2; // TextureCenterCoordinates (vec2), VirtualTextureCoordinates (vec2) 
in vec2 inCloud3; // Darkening, TotalDistanceTraveled

// Outputs
out vec2 texturePos;
out vec2 textureCenterPos;
out vec2 virtualTexturePos;
out float darkness;
out float totalDistanceTraveled;

void main()
{
    texturePos = inCloud1.zw;
    textureCenterPos = inCloud2.xy;
    virtualTexturePos = inCloud2.zw;
    darkness = inCloud3.x;
    totalDistanceTraveled = inCloud3.y;

    gl_Position = vec4(inCloud1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 texturePos;
in vec2 textureCenterPos;
in vec2 virtualTexturePos;
in float darkness;
in float totalDistanceTraveled;

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

    // (r, a) (r=[0.0, 1.0], a=[0.0, 1.0 CCW from W])
    #define PI 3.1415926
    vec2 ra = vec2(
        length(virtualTextureCoordsCentered), 
        atan(virtualTextureCoordsCentered.y, virtualTextureCoordsCentered.x));
    //#define SPEED 1.2
    #define SPEED 3.2
    vec2 ra2 = vec2(ra.x - totalDistanceTraveled * SPEED, ra.y / PI);
    #define MAG .5
    float n = texture2D(paramNoiseTexture, ra2 * MAG).x;
    //float a = 1. - n * n * ra.x * ra.x;
    //float a = 1. - n * ra.x; 
    float a = 1. - abs(n) * smoothstep(0.2, 0.4, ra.x); // Silence towards center
    #define VAR 0.28
    a = (1.-VAR) + VAR*smoothstep(0.0, 0.8, a);
    gl_FragColor = vec4(
        cloudColor,
        textureColor.a * a);
        /*
    gl_FragColor = vec4(
        a, a, a,
        1.0);
*/

    // TODOOLD
    /*
    // Sample alpha
    //vec2 alphaMaskTexturePos = textureCenterPos + (texturePos - textureCenterPos) * growthProgress;
    //vec4 alphaMaskSample = texture2D(paramCloudsAtlasTexture, alphaMaskTexturePos);
    vec2 alphaMaskTexturePos = textureCenterPos + (texturePos - textureCenterPos) * growthProgress;
    float alphaMaskSample = texture2D(paramNoiseTexture, alphaMaskTexturePos * 5.).x;
    #define NOISE_AMPL .8
    float m = (1.0 - NOISE_AMPL) + alphaMaskSample * NOISE_AMPL;
       
    // Combine into final color
    float alpha = sqrt(textureColor.w * m);
    gl_FragColor = vec4(cloudColor, alpha);
    */
} 
