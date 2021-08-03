###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inPressureInjectionHalo1;  // Position, HaloSpacePosition (-1,1)
in float inPressureInjectionHalo2;  // FlowMultiplier

// Outputs
out vec2 haloSpacePosition;
out float flowMultiplier;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    haloSpacePosition = inPressureInjectionHalo1.zw;
    flowMultiplier = inPressureInjectionHalo2;

    gl_Position = paramOrthoMatrix * vec4(inPressureInjectionHalo1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 haloSpacePosition; // (x=[-1.0, 1.0], y=[-1.0, 1.0])
in float flowMultiplier;

// Parameters
uniform float paramTime;

void main()
{
    #define T 1.
    #define RADIUS 1.
    #define THICKNESS .4

    // Where in a half period
    float ht = fract(paramTime / (T / 2. * flowMultiplier));
    
    float radius1 = (RADIUS / 2.) * ht;
    float radius2 = RADIUS / 2. + (RADIUS / 2.) * ht;
    
    float alpha1 = 1.;
    float alpha2 = 1. - ht;
    
    //
    // Render
    //
        
    float d = length(haloSpacePosition);
    
    float w1 = 
        smoothstep(radius1 - THICKNESS / 2., radius1, d)
        - smoothstep(radius1, radius1 + THICKNESS / 2., d);

    float w2 = 
        smoothstep(radius2 - THICKNESS / 2., radius2, d)
        - smoothstep(radius2, radius2 + THICKNESS / 2., d);

    float whiteDepth = max(w1 * alpha1, w2 * alpha2) * .7;
    
    vec4 c1 = vec4(whiteDepth, whiteDepth, whiteDepth, 1.); // white
    
    ///////

    gl_FragColor = vec4(whiteDepth, whiteDepth, whiteDepth, 1.);
}
