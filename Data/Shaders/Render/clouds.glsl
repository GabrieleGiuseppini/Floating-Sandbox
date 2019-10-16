###VERTEX

#version 120

#define in attribute
#define out varying

//
// Clouds' inputs are directly in NDC coordinates
//

// Inputs
in vec4 inCloud; // Position (vec2), TextureCoordinates (vec2)

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = vec4(inCloud.xy, -1.0, 1.0);
    texturePos = inCloud.zw;
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 texturePos;

// The texture
uniform sampler2D paramCloudTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramCloudDarkening;

void main()
{
    vec4 textureColor = texture2D(paramCloudTexture, texturePos);
    gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity * paramCloudDarkening, textureColor.w);
} 
