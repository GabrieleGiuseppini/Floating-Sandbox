###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inNpcTextureAttributeGroup1; // Position, TextureCoords
in vec3 inNpcTextureAttributeGroup2; // PlaneId
in vec4 inNpcTextureAttributeGroup3; // OverlayColor

// Outputs        
out vec2 textureCoords;
out vec4 vertexOverlayColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    textureCoords = inNpcTextureAttributeGroup1.zw;
    vertexOverlayColor = inNpcTextureAttributeGroup3;

    gl_Position = paramOrthoMatrix * vec4(inNpcTextureAttributeGroup1.xy, inNpcTextureAttributeGroup2.x, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader        
in vec2 textureCoords;
in vec4 vertexOverlayColor;

// The texture
uniform sampler2D paramNpcAtlasTexture;

// Params
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    vec4 c = texture2D(paramNpcAtlasTexture, textureCoords);

    // TODOOLD
    /*
    vec2 uv = vec2(pow(vertexSpacePosition.x, 3.0), vertexSpacePosition.y);
    
    float d = distance(uv, vec2(.0, .0));    
    float alpha = 1.0 - smoothstep(0.9, 1.1, d);
    float borderAlpha = alpha - (1.0 - smoothstep(0.55, 0.8, d));

    // 1.0 when in direction X, 0.0 otherwise
    #define MIN_SHADE 0.3
    float x2 = (vertexSpacePosition.x - vertexOrientationDepth);
    float oppositeDirShade = MIN_SHADE + (1.0 - MIN_SHADE) * (1.0 - x2 * x2 / (2.0 * (vertexOrientationDepth + 1)));
    
    vec3 cInner = vec3(0.560, 0.788, 0.950) * (1.0 - vertexBackDepth / 2.0);
    vec3 cBorder = vec3(0.10, 0.10, 0.10);
    vec4 c = vec4(
        mix(cInner, cBorder, borderAlpha) * oppositeDirShade,
        alpha);
    */

    // Luminosity blend
    float l = (c.r + c.g + c.b) / 3.0;
    c = vec4(
        mix(
            c.rgb,
            l * vertexOverlayColor.rgb,
            vertexOverlayColor.a),
        c.a);

    // Apply ambient light
    c.rgb *= paramEffectiveAmbientLightIntensity;        

    gl_FragColor = c;
} 
