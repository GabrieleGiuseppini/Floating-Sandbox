###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inFish1; // CenterPosition (vec2), VertexOffset (vec2)
in vec4 inFish2; // TextureCoordinates (vec2), TextureCoordinatesXLimits (vec2)
in vec3 inFish3; // AngleCw (float), TailX (float), TailProgress (float)

// Outputs
out vec2 vertexTextureCoordinates;
out float worldY;
out vec2 vertexTextureCoordinatesXLimits;
out float tailX;
out float tailProgress;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inFish2.xy;
    worldY = inFish1.y;
    vertexTextureCoordinatesXLimits = inFish2.zw;
    tailX = inFish3.y;
    tailProgress = inFish3.z;
    
    float angleCw = inFish3.x;

    mat2 rotationMatrix = mat2(
        cos(angleCw), -sin(angleCw),
        sin(angleCw), cos(angleCw));

    vec2 worldPosition = 
        inFish1.xy 
        + rotationMatrix * inFish1.zw;

    gl_Position = paramOrthoMatrix * vec4(worldPosition.xy, -1.0, 1.0);

    
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;
in float worldY;
in vec2 vertexTextureCoordinatesXLimits;
in float tailX;
in float tailProgress;

// The texture
uniform sampler2D paramFishesAtlasTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanDarkeningRate;

void main()
{
    //
    // Here we simulate a bar bending around the y axis at x=TailX,
    // and rendered with perspective
    //
    
    //
    // Calculate angle: [-Alpha0 -> Alpha0]
    //
    
    #define Alpha0 0.5
    
    float alpha = Alpha0 * tailProgress;
    
    //
    // Calculate Z simulating pivoting aroind tailX
    //
    // Z = Z(alpha, x) : [-Tailx * sin(Alpha0), TailX * sin(Alpha0)]
    //
    
    // Smoothstep range around tailX: tailX-LMargin -> tailX+RMargin
    #define LTailMargin .6
    #define RTailMargin .0
    
    float z = 
        (tailX - vertexTextureCoordinates.x) * sin(alpha) 
        * smoothstep(-RTailMargin, LTailMargin, tailX - vertexTextureCoordinates.x); // Only apply to left side of tail, i.e. where x < tailX + RTailMargin; else z=0.0
        
    // Shift Z so that when Z is closest to viewer, texture is at normal size
    //
    // Z(alpha, @x=0) = tailX * sin(alpha)
    z -= tailX * sin(Alpha0) + .0;
    
    #define Z0 4.1
    
    //
    // Transform X: perspective + rotation
    //
    
    float perspectiveDivisor = (Z0 + z);
    
    float rotationDivisor = cos(
        alpha 
        * smoothstep(-RTailMargin, LTailMargin, tailX - vertexTextureCoordinates.x)); // Only apply to left side of tail, i.e. where x < tailX + RTailMargin; else divisor=1.0
    
    float textureX = clamp(
        vertexTextureCoordinatesXLimits.x, // L margin
        vertexTextureCoordinatesXLimits.y, // R margin
        tailX + Z0 * (vertexTextureCoordinates.x - tailX) / (perspectiveDivisor * rotationDivisor));

        
   	//
    // Transform Y: perspective
    //
    
    float textureY = 
        .5
        + Z0 * (vertexTextureCoordinates.y - .5) / (Z0 + z);
        
    //
    // Sample texture
    //
    
    vec2 tc2 = vec2(
        textureX,
        textureY);

    /////////////////////////////////////////////////////

    vec4 fishSample = texture2D(paramFishesAtlasTexture, tc2);

    float darkMix = 1.0 - exp(min(0.0, worldY) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)

    vec4 textureColor = mix(
        fishSample,
        vec4(0,0,0,0), 
        pow(darkMix, 3.0));

    gl_FragColor = vec4(
        textureColor.xyz * paramEffectiveAmbientLightIntensity,
        textureColor.w);
} 
