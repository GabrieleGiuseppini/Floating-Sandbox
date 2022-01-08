###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inTexture; // Vertex position (ship space), Texture coords (texture space)

// Outputs
out vec2 vertexTextureCoordinates;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexTextureCoordinates = inTexture.zw;

    gl_Position = paramOrthoMatrix * vec4(inTexture.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexTextureCoordinates;

// The texture
uniform sampler2D paramTextureUnit1;

// Params
uniform float paramOpacity;
uniform vec2 paramPixelsPerShipParticle;
uniform vec2 paramShipParticleTextureSize; // Size of one ship particle in texture space (i.e. in the [0, 1] space), separately for x and y

void main()
{
    // Calculate quantized texture coords - coords of bottom-left corner
    vec2 quvBL = floor(vertexTextureCoordinates / paramShipParticleTextureSize) * paramShipParticleTextureSize;
    
    // Calculate fractional position of this pixel's position in this ship particle's square, wrt BL
    vec2 fBL = (vertexTextureCoordinates - quvBL);
    
    // Normalized fractional position
    vec2 fBLn = fBL / paramShipParticleTextureSize;
    
    // Given that the texture is setup for nearest, we offset the UV coords so that we sample instead
    // smack in the middle of the ship particle
    vec2 quvSample = quvBL + paramShipParticleTextureSize / 2.0;
    
    // Line thickness in the normalized fractional position space
    vec2 lineThickness = 
        0.51 // Pixels on either side
        / paramPixelsPerShipParticle;
    float lineThicknessD = length(lineThickness);
    
    //
    // Vertices
    //
    // 1-2-3  +
    // |\|/|  |
    // 4-5-6  
    // |/|\|  |
    // 7-8-9  -
    //
    
    // Pre-calc "booleans" for bound checks
    float hasXMinus = step(0.0, quvSample.x - paramShipParticleTextureSize.x);
    float hasXPlus = step(quvSample.x + paramShipParticleTextureSize.x, 1.0);
    float hasYMinus = step(0.0, quvSample.y - paramShipParticleTextureSize.y);
    float hasYPlus = step(quvSample.y + paramShipParticleTextureSize.y, 1.0);
    
    // 1
    vec2 particle1quv = quvSample + vec2(-paramShipParticleTextureSize.x, paramShipParticleTextureSize.y);
    float hasVertex1 = texture2D(paramTextureUnit1, particle1quv).w;
        
    // 2
    vec2 particle2quv = quvSample + vec2(0., paramShipParticleTextureSize.y);
    float hasVertex2 = texture2D(paramTextureUnit1, particle2quv).w;        

    // 3
    vec2 particle3quv = quvSample + vec2(paramShipParticleTextureSize.x, paramShipParticleTextureSize.y);
    float hasVertex3 = texture2D(paramTextureUnit1, particle3quv).w;
        
    // 4
    vec2 particle4quv = quvSample + vec2(-paramShipParticleTextureSize.x, 0.);
    float hasVertex4 = texture2D(paramTextureUnit1, particle4quv).w;              

    // 5
    vec4 particle5Color = texture2D(paramTextureUnit1, quvSample);
    float hasVertex5 = particle5Color.w;
        
    // 6
    vec2 particle6quv = quvSample + vec2(paramShipParticleTextureSize.x, 0.);
    float hasVertex6 = texture2D(paramTextureUnit1, particle6quv).w;          
    
    // 7
    vec2 particle7quv = quvSample + vec2(-paramShipParticleTextureSize.x, -paramShipParticleTextureSize.y);
    float hasVertex7 = texture2D(paramTextureUnit1, particle7quv).w;

    // 8
    vec2 particle8quv = quvSample + vec2(0., -paramShipParticleTextureSize.y);
    float hasVertex8 = texture2D(paramTextureUnit1, particle8quv).w;

    // 9
    vec2 particle9quv = quvSample + vec2(paramShipParticleTextureSize.x, -paramShipParticleTextureSize.y);
    float hasVertex9 = texture2D(paramTextureUnit1, particle9quv).w;

    //
    // Lines
    //
    
    // 1
    //  \
    //   5
    //    \
    //     9
    float isOnLine19 = step(abs(1.0 - fBLn.y - fBLn.x), lineThicknessD);
    
    //   2
    //   |
    //   5
    //   |
    //   8
    float isOnLine28 = step(abs(0.5 - fBLn.x), lineThickness.x);

    //     3
    //    /  
    //   5
    //  /
    // 7
    float isOnLine37 = step(abs(fBLn.y - fBLn.x), lineThicknessD);
    
    //
    //
    // 4-5-6
    //
    //
    float isOnLine46 = step(abs(0.5 - fBLn.y), lineThickness.y);
    
    //
    // Combine
    //
    
    float isOnUpperLines =
        step(0.5, fBLn.y)
        * hasYPlus
        * (hasVertex1 * hasXMinus * isOnLine19 + hasVertex2 * isOnLine28 + hasVertex3 * hasXPlus * isOnLine37);
    
    float isOnLeftLines =
        step(fBLn.x, 0.5)
        * hasXMinus
        * (hasVertex4 * isOnLine46);

    float isOnRightLines =
        step(0.5, fBLn.x)
        * hasXPlus
        * (hasVertex6 * isOnLine46);
    
    float isOnLowerLines =
        step(fBLn.y, 0.5)
        * hasYMinus
        * (hasVertex7 * hasXMinus * isOnLine37 + hasVertex8 * isOnLine28 + hasVertex9 * hasXPlus * isOnLine19);
                
    //
    // Particle
    //
    
    # define ParticleSize .15
    float isOnParticle = 1.0 - smoothstep(ParticleSize - ParticleSize/3., ParticleSize + ParticleSize/3., length(fBLn - .5));

    //
    // Combine outputs
    //
    
    gl_FragColor = vec4(
        particle5Color.xyz,
        hasVertex5 
        * min(
            1.0, 
            isOnParticle + isOnUpperLines + isOnLowerLines + isOnLeftLines + isOnRightLines)
        * paramOpacity);
}
