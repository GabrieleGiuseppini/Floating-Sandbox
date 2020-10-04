###VERTEX

#version 120

#define in attribute
#define out varying

//
// Clouds' inputs are directly in NDC coordinates
//

// Inputs
in vec4 inCloud1; // Position (vec2), TextureCoordinates (vec2)
in vec3 inCloud2; // AlphaMaskTextureCoordinates (vec2), Darkening

// Outputs
out vec2 texturePos;
out vec2 alphaMaskTexturePos;
out float darkness;

void main()
{
    texturePos = inCloud1.zw;
    alphaMaskTexturePos = inCloud2.xy;
    darkness = inCloud2.z;

    gl_Position = vec4(inCloud1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 texturePos;
in vec2 alphaMaskTexturePos;
in float darkness;

// The texture
uniform sampler2D paramCloudsAtlasTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    vec4 alphaMaskSample = texture2D(paramCloudsAtlasTexture, alphaMaskTexturePos);
    vec4 textureColor = texture2D(paramCloudsAtlasTexture, texturePos);
    //gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity * darkness, textureColor.w * alphaMaskSample.w);
    //gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity * darkness, textureColor.w);
    gl_FragColor = vec4(alphaMaskSample.xyz * paramEffectiveAmbientLightIntensity * darkness, alphaMaskSample.w);
} 
