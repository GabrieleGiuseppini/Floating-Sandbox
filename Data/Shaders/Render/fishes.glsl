###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inFish1; // CenterPosition (vec2), VertexOffset (vec2)
in vec3 inFish2; // TextureCoordinates (vec2), AngleCw (float)

// Outputs
out vec2 vertexTextureCoordinates;
out float worldY;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inFish2.xy; 
    
    float angleCw = inFish2.z;

    mat2 rotationMatrix = mat2(
        cos(angleCw), -sin(angleCw),
        sin(angleCw), cos(angleCw));

    vec2 worldPosition = 
        inFish1.xy 
        + rotationMatrix * inFish1.zw;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, -1.0, 1.0);

    worldY = gl_Position.y;
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float worldY;

// The texture
uniform sampler2D paramFishesAtlasTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanDarkeningRate;

void main()
{
    vec4 fishSample = texture2D(paramFishesAtlasTexture, vertexTextureCoordinates);

    float darkMix = 1.0 - exp(min(0.0, worldY) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)

    vec4 textureColor = mix(
        fishSample,
        vec4(0,0,0,0), 
        pow(darkMix, 3.0));

    gl_FragColor = vec4(
        textureColor.xyz * paramEffectiveAmbientLightIntensity,
        textureColor.w);
} 
