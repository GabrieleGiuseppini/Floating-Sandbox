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
uniform float paramAmbientLightIntensity;

// The texture
uniform sampler2D sharedSpringTexture;

void main()
{
    vec4 textureColor = texture2D(sharedSpringTexture, vertexTextureCoordinates);

    //vec3 textColor =
    //    textureColor.xyz * sqrt(paramAmbientLightIntensity)
    //    + vec3(1.0, 1.0, 1.0) * (1.0 - sqrt(paramAmbientLightIntensity));

    vec3 textColor =
        textureColor.xyz * step(0.5, paramAmbientLightIntensity)
        + vec3(1.0, 1.0, 1.0) * (1.0 - step(0.5, paramAmbientLightIntensity));

    gl_FragColor = vec4(
        textColor,
        textureColor.w * vertexAlpha);
} 
