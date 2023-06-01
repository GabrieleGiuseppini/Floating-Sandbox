###VERTEX-120

#define in attribute
#define out varying

//
// Clouds' inputs are directly in NDC coordinates
//

// Inputs
in vec4 inCloud1; // Position (NDC) (vec2), TextureCoordinates (vec2)
in vec4 inCloud2; // TextureCenterCoordinates (vec2), Darkening, GrowthProgress

// Outputs
out vec2 texturePos;
out vec2 textureCenterPos;
out float darkness;
out float growthProgress;

void main()
{
    texturePos = inCloud1.zw;
    textureCenterPos = inCloud2.xy;
    darkness = inCloud2.z;
    growthProgress = inCloud2.w;

    gl_Position = vec4(inCloud1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 texturePos;
in vec2 textureCenterPos;
in float darkness;
in float growthProgress;

// The texture
uniform sampler2D paramCloudsAtlasTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;

void main()
{
    // Sample texture
    vec4 textureColor = texture2D(paramCloudsAtlasTexture, texturePos);

    // Adjust for ambient light and mooncolor and (storm) darkness
    vec3 cloudColor = textureColor.xyz * mix(paramEffectiveMoonlightColor, vec3(1.), paramEffectiveAmbientLightIntensity) * darkness;

    // Sample alpha
    vec2 alphaMaskTexturePos = textureCenterPos + (texturePos - textureCenterPos) * growthProgress;
    vec4 alphaMaskSample = texture2D(paramCloudsAtlasTexture, alphaMaskTexturePos);
       
    // Combine into final color
    float alpha = sqrt(textureColor.w * alphaMaskSample.w);
    gl_FragColor = vec4(cloudColor, alpha);
} 
