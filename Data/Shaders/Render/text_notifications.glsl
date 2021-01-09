###VERTEX-120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inTextNotification1;  // Position (vec2), TextureCoordinates (vec2)
in float inTextNotification2; // Alpha

// Outputs
out vec2 vertexTextureCoordinates;
out float vertexAlpha;

void main()
{
    vertexTextureCoordinates = inTextNotification1.zw; 
    vertexAlpha = inTextNotification2;
    gl_Position = vec4(inTextNotification1.xy, -1.0, 1.0);
}


###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float vertexAlpha;

// Params
uniform float paramTextLighteningStrength;

// The texture
uniform sampler2D paramSharedTexture;

void main()
{
    vec4 textureColor = texture2D(paramSharedTexture, vertexTextureCoordinates);

    vec3 textColor = mix(textureColor.xyz, vec3(0.8, 0.8, 0.8), paramTextLighteningStrength);

    gl_FragColor = vec4(
        textColor,
        textureColor.w * vertexAlpha);
} 
