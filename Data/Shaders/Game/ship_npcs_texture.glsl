###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inNpcTextureAttributeGroup1; // Position, VertexSpacePosition
in vec3 inNpcTextureAttributeGroup2; // PlaneId, BackDepth, OrientationDepth
in vec4 inNpcTextureAttributeGroup3; // OverlayColor

// Outputs        
out vec2 vertexSpacePosition;
out float vertexBackDepth;
out float vertexOrientationDepth;
out vec4 vertexOverlayColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexSpacePosition = inNpcTextureAttributeGroup1.zw;
    vertexBackDepth = inNpcTextureAttributeGroup2.y;
    vertexOrientationDepth = inNpcTextureAttributeGroup2.z;
    vertexOverlayColor = inNpcTextureAttributeGroup3;

    gl_Position = paramOrthoMatrix * vec4(inNpcTextureAttributeGroup1.xy, inNpcTextureAttributeGroup2.x, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader        
in vec2 vertexSpacePosition; // [(-1.0, -1.0), (1.0, 1.0)]
in float vertexBackDepth;
in float vertexOrientationDepth;
in vec4 vertexOverlayColor;

// Params
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
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
