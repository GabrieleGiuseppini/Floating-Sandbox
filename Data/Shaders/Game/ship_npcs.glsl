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
    
    #define Radius 0.95
    #define BorderWidth 0.03
    
    float bodyDepth = step(r, Radius - BorderWidth);
    
    float borderDepth = 
        step(Radius - BorderWidth, r)
        * (1.0 - smoothstep(Radius - BorderWidth / 2.0, Radius, r));

    vec4 col = vec4(
        (vec3(0.870, 0.855, 0.00) * bodyDepth + vec3(0.3, 0.3, 0.3) * borderDepth) * paramEffectiveAmbientLightIntensity,
        step(r, Radius));
        
    // Add highlight
    float highlightDepth = step(r, 1.0);
    col = 
        col * col.a
        + vertexHighlightColor 
            * step(Radius, r) 
            * (1.0 - smoothstep(Radius + (1.0 - Radius) / 2.0,  1.0, r));

    ////////////////////

    gl_FragColor = col;
} 
