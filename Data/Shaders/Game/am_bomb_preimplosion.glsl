###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inAMBombPreImplosion1;  // Position, CenterPosition
in vec2 inAMBombPreImplosion2; // Progress, Radius

// Outputs
out vec2 vertexWorldCoords;
out vec2 vertexWorldCenterPosition;
out float vertexProgress;
out float vertexWorldRadius;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexWorldCoords = inAMBombPreImplosion1.xy;
    vertexWorldCenterPosition = inAMBombPreImplosion1.zw;

    vertexProgress = inAMBombPreImplosion2.x;
    vertexWorldRadius = inAMBombPreImplosion2.y;

    gl_Position = paramOrthoMatrix * vec4(inAMBombPreImplosion1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexWorldCoords;
in vec2 vertexWorldCenterPosition;
in float vertexProgress;
in float vertexWorldRadius;

void main()
{
    float d = distance(vertexWorldCoords, vertexWorldCenterPosition) / vertexWorldRadius;

    #define HaloWidth .012

    float halo = 
        smoothstep(1. - 3. * HaloWidth, 1. - HaloWidth, d)
        - smoothstep(1. - HaloWidth / 2., 1., d);
    halo *= .15;
       
    float waves = .05 * abs(sin((1. - d) * 24. * (1. + d)));
    
    float fadeout = 1. - smoothstep(.75, 1., vertexProgress);
        
    gl_FragColor = vec4(204./255., 239./255., 240./255., (halo + waves) * fadeout * step(d, 1.0));
}
