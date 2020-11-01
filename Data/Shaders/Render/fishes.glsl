###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inFish1; // CenterPosition (vec2), VertexOffset (vec2)
in vec4 inFish2; // TextureSpace L, B, R, T (vec4)
in vec4 inFish3; // TextureCoordinates (vec2),AngleCw (float), TailX (float)
in vec2 inFish4; // TailSwing (float), TailProgress (float)

// Outputs
out vec2 vertexTextureCoordinates;
out float worldY;
out vec4 vertexTextureSpace;
out float tailXNorm;
out float tailSwing;
out float tailProgress;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inFish3.xy;
    worldY = inFish1.y;
    vertexTextureSpace = inFish2;
    tailXNorm = inFish3.w;
    tailSwing = inFish4.x;
    tailProgress = inFish4.y;
    
    float angleCw = inFish3.z;

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
in vec4 vertexTextureSpace;
in float tailXNorm;
in float tailSwing;
in float tailProgress;

// The texture
uniform sampler2D paramFishesAtlasTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramOceanDarkeningRate;

void main()
{
    /////////////////////////////////////////////////////////
    
    //
    // Here we simulate a bar bending around the y axis at x=TailX,
    // and rendered with perspective
    //
    
    // In texture space
    float tailX = vertexTextureSpace.x + (vertexTextureSpace.z - vertexTextureSpace.x) * tailXNorm;
    
    float xL = vertexTextureSpace.x;
    float yB = vertexTextureSpace.y;
    float xR = vertexTextureSpace.z;    
    float yT = vertexTextureSpace.w;
    
    //
    // Calculate angle: 
    //  - At left end: [-tailSwing -> tailSwing]
    //	- Beyond tailX + epsilon: 0.0
    //
    
    float smoothRMargin = tailX + .2 * (xR - tailX);
    
    float alpha = 
        tailSwing * tailProgress 
        * (1.0 - smoothstep(xL, smoothRMargin, vertexTextureCoordinates.x));
    
    //alpha = -0.2; // TODOTEST
    
    // The depth at which the fish is at rest
    #define Z0 1.
    
    // Calculate Z: rotation around TailX of a bar of length |tailX - x|
    float z = Z0 + (vertexTextureCoordinates.x - tailX) * sin(alpha);
    
    // Calculate texture X: inverse of rotation around TailX of a bar of length (x - LimitX)
    float x = (vertexTextureCoordinates.x - tailX) / cos(alpha) + tailX;
    
    //
    // Transform X and Y for perspective
    //           
    
    float midX = (xL + xR) / 2.0;

    float textureX = clamp(
        midX + (x - midX) * z,
        xL,
        xR);
        
    
    float midY = (yB + yT) / 2.0;
    
    float textureY = clamp(
        midY + (vertexTextureCoordinates.y - midY) * z,
        yB,
        yT);    
        
    //
    // Sample texture
    //
    
    vec2 tc2 = vec2(
        textureX,
        textureY);
    
    /////////////////////////////////////////////////////////

    vec4 fishSample = texture2D(paramFishesAtlasTexture, tc2);

    float darkMix = 1.0 - exp(min(0.0, worldY) * paramOceanDarkeningRate); // Darkening is based on world Y (more negative Y, more dark)

    vec4 textureColor = mix(
        fishSample,
        vec4(0,0,0,0), 
        pow(darkMix, 3.0));

    gl_FragColor = vec4(
        textureColor.xyz * paramEffectiveAmbientLightIntensity,
        fishSample.w);
} 
