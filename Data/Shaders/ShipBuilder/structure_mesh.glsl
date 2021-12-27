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
uniform vec2 paramShipParticleTextureSize;

void main()
{
    // Calculate quantized texture coords
    vec2 quv = floor(vertexTextureCoordinates / paramShipParticleTextureSize) * paramShipParticleTextureSize;
    
    // Calculate fractional position of this pixel's position in this ship particle's square
    vec2 f = vertexTextureCoordinates - quv;
        
    // Vertices
    vec4 particle1Color = texture2D(paramTextureUnit1, quv);
    float hasVertex1 = particle1Color.w;
    vec4 particle2Color = texture2D(paramTextureUnit1, quv + vec2(paramShipParticleTextureSize.x, 0.0));
    float hasVertex2 = particle2Color.w;
    float hasVertex3 = texture2D(paramTextureUnit1, quv + vec2(0.0, paramShipParticleTextureSize.y)).w;
    float hasVertex4 = texture2D(paramTextureUnit1, quv + paramShipParticleTextureSize).w;
    
    vec2 lineThickness = paramShipParticleTextureSize / 8.0;
    float lineThicknessD = length(lineThickness) / 2.0;
    
    // Particle: depth=1 when in the bottom-left corner of the ship particle square,
    // and when no 1-* lines at all
    vec2 particleSize = paramShipParticleTextureSize / 3.0;
    float particleDepth = hasVertex1
        * step(f.x, particleSize.x)
        * step(f.y, particleSize.y)
        // TODOTEST
        ;
        //* (1.0 - min(1.0, hasVertex2 + hasVertex3 + hasVertex4));
    
    // 1---2
    float line12Depth = (hasVertex1 * hasVertex2)
        * step(f.y, lineThickness.y);

    // 3
    // |
    // 1
    float line13Depth = (hasVertex1 * hasVertex3)
        * step(f.x, lineThickness.x);
        
    //     4
    //   /
    // 1
    float line14Depth = (hasVertex1 * hasVertex4)
        * step(abs(f.y - f.x), lineThicknessD);
        
    // 3
    //   \
    //     2
    float line23Depth = (hasVertex2 * hasVertex3)
        * step(abs(paramShipParticleTextureSize.y + lineThickness.y - f.y - f.x), lineThicknessD);
        
    //
    // Combine outputs
    //
    
    vec4 color1 = vec4(
        particle1Color.xyz,
        min(1.0, particleDepth + line12Depth + line13Depth + line14Depth));
        
    vec4 color2 = vec4(
        particle2Color.xyz,
        line23Depth);
        
    gl_FragColor = mix(
        color1,
        color2,
        color2.w * paramOpacity);
} 
