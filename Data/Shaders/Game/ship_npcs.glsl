###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inNpcQuad1; // VertexPosition (vec2), QuadSpacePosition (vec2)
in float inNpcQuad2; // PlaneId
in vec4 inNpcStaticAttributeGroup1; // HighlightColor (vec4)

// Outputs
out vec2 vertexPositionNpc;
out vec2 vertexQuadSpacePosition;
out vec4 vertexHighlightColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inNpcQuad1.xy, inNpcQuad2, 1.0);

    vertexPositionNpc = gl_Position.xy;
    vertexQuadSpacePosition = inNpcQuad1.zw;
    vertexHighlightColor = inNpcStaticAttributeGroup1;
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader        
in vec2 vertexPositionNpc;
in vec2 vertexQuadSpacePosition;
in vec4 vertexHighlightColor;

// Params
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    float r = distance(vertexQuadSpacePosition, vec2(.0, .0));
    
    #define NpcRadius 0.90
    #define BorderWidth 0.03
    
    // Border goes half inside and half outside of NpcRadius
    
    float bodyDepth = step(r, NpcRadius);
    
    float borderDepth = 
        smoothstep(NpcRadius - BorderWidth / 2.0, NpcRadius - BorderWidth / 4.0, r)
        * (1.0 - smoothstep(NpcRadius + BorderWidth / 4.0, NpcRadius + BorderWidth / 2.0, r));

    vec4 col = mix(
        vec4(0.870, 0.855, 0.00, bodyDepth),
        vec4(0.3, 0.3, 0.3, borderDepth),
        borderDepth);
        
        
    // Add highlight
    
    #define HighlightWidth 0.1
    
    float highlightDepth = 
        (1.0 - smoothstep(1.0 - BorderWidth / 4.0, 1.0, r))
        * vertexHighlightColor.a;
        
    col = mix(
        col,
        vertexHighlightColor,
        highlightDepth - col.a);

    // Apply ambient light

    col.rgb *= paramEffectiveAmbientLightIntensity;

    ////////////////////

    gl_FragColor = col;
} 
