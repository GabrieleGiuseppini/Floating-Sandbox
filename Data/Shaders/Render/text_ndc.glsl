###VERTEX

#version 120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inText1;  // Position (vec2), TextureCoordinates (vec2)
in float inText2; // Alpha

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;

void main()
{
    vertexTextureCoordinates = inText1.zw; 
    vertexAlpha = inText2;
    gl_Position = vec4(inText1.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;

// Params
uniform float paramEffectiveAmbientLightIntensity;

// The texture
uniform sampler2D paramSharedTexture;

void main()
{
    vec4 textureColor = texture2D(paramSharedTexture, vertexTextureCoordinates);

    vec3 textColor =
        textureColor.xyz * step(0.5, paramEffectiveAmbientLightIntensity)
        + vec3(1.0, 1.0, 1.0) * (1.0 - step(0.5, paramEffectiveAmbientLightIntensity));

    gl_FragColor = vec4(
        textColor,
        textureColor.w * vertexAlpha);
} 
