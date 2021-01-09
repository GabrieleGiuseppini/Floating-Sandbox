###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec3 inLand; // Position (vec2), Depth (float) (0 at top, +X at bottom)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec3 textureCoord;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inLand.xy, -1.0, 1.0);
    textureCoord = inLand;
}


###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec3 textureCoord;

// The texture
uniform sampler2D paramLandTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec2 paramTextureScaling;
uniform float paramOceanDarkeningRate;

void main()
{
    // Back-sample 10.0 at top, slowly going to zero
    float sampleIncrement = -10.0 * (2.0 - 2.0 * smoothstep(-3.0, 3.0, textureCoord.z));
    vec2 textureCoord2 = vec2(textureCoord.x, -textureCoord.y + sampleIncrement);

    float darkMix = 1.0 - exp(min(0.0, textureCoord.y) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)
    vec4 textureColor = mix(
        texture2D(paramLandTexture, textureCoord2 * paramTextureScaling), 
        vec4(0,0,0,0), 
        pow(darkMix, 3.0));

    gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity, 1.0);
} 
