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
uniform vec2 paramShipParticleTextureSize; // Size of one ship particle in texture space (i.e. in the [0, 1] space), separately for x and y

void main()
{
    // Calculate quantized texture coords
    vec2 quv = floor(vertexTextureCoordinates / paramShipParticleTextureSize) * paramShipParticleTextureSize;

    // Calculate fractional position of this pixel's position in this ship particle's square
    vec2 f = vertexTextureCoordinates - quv;

    // Given that the texture is setup for nearest, we offset the UV coords so that we sample instead
    // smack in the middle of the texture
    quv += paramShipParticleTextureSize / 2.0;

    // Line thickness
    vec2 lineThickness = paramShipParticleTextureSize / 8.0;
    float halfLineThicknessD = length(lineThickness) / 2.0;

    //
    // Vertices
    //

    vec4 particle1Color = texture2D(paramTextureUnit1, quv);
    float hasVertex1 = particle1Color.w;

    vec2 particle2quv = quv + vec2(paramShipParticleTextureSize.x, 0.);
    vec4 particle2Color = texture2D(paramTextureUnit1, particle2quv);
    float hasVertex2 =
        particle2Color.w
        * step(particle2quv.x, 1.0);

    vec2 particle3quv = quv + vec2(0., paramShipParticleTextureSize.y);
    float hasVertex3 =
        texture2D(paramTextureUnit1, particle3quv).w
        * step(particle3quv.y, 1.0);

    vec2 particle4quv = quv + paramShipParticleTextureSize;
    float hasVertex4 =
        texture2D(paramTextureUnit1, particle4quv).w
        * step(particle4quv.x, 1.0)
        * step(particle4quv.y, 1.0);

    //
    // Particle
    //

    // Depth=1 when in the bottom-left corner of the ship particle square,
    // and when no 1-3 and 1-4 lines
    vec2 particleSize = paramShipParticleTextureSize / 3.0;
    float particleDepth = hasVertex1
        * step(f.x, particleSize.x)
        * step(f.y, particleSize.y)
        * (1.0 - min(1.0, hasVertex3 + hasVertex4));

    //
    // Lines
    //

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
        * step(abs(f.y - paramShipParticleTextureSize.y / paramShipParticleTextureSize.x * f.x), halfLineThicknessD);

    // 3
    //   \
    //     2
    float line23Depth = (hasVertex2 * hasVertex3)
        * step(abs(paramShipParticleTextureSize.y + lineThickness.y - f.y - paramShipParticleTextureSize.y / paramShipParticleTextureSize.x * f.x), halfLineThicknessD);

    //
    // Combine outputs
    //

    vec4 color1 = vec4(
        particle1Color.xyz,
        min(1.0, particleDepth + line12Depth + line13Depth + line14Depth) * paramOpacity);

    vec4 color2 = vec4(
        particle2Color.xyz,
        line23Depth * paramOpacity);

    gl_FragColor = mix(
        color1,
        color2,
        color2.w);
}
