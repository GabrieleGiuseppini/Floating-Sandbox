###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inMultiNotification1;  // Type, WorldPosition (vec2), [FlowMultiplier] float1 (float)
in vec2 inMultiNotification2; // VirtualSpacePosition (vec2)

// Outputs
out float notification_type;
out float float1;
out vec2 virtualSpacePosition;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    notification_type = inMultiNotification1.x;
    float1 = inMultiNotification1.w;
    virtualSpacePosition = inMultiNotification2.xy;

    gl_Position = paramOrthoMatrix * vec4(inMultiNotification1.yz, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float notification_type;
in float float1; // FlowMultiplier
in vec2 virtualSpacePosition; // [-1.0, 1.0]

// Parameters
uniform float paramTime;

// TODOHERE
// The texture
//uniform sampler2D paramNoiseTexture;

float is_type(float notification_type, float value)
{
    return step(value - 0.5, notification_type) * step(notification_type, value + 0.5);
}

vec4 make_grip_circle(float d)
{
    #define MAX_ALPHA 0.3

    float alpha = MAX_ALPHA * (1.0 - smoothstep(0.95, 1.0, d));

    return vec4(0.0, 0.0, 0.0, alpha);
}

vec4 make_pressure_injection_halo(
    float d,
    float flowMultiplier)
{
    #define T 1.
    #define RADIUS 1.
    #define THICKNESS .5

    // Where in a half period
    float ht = fract(paramTime / (T / 2. * flowMultiplier));
    
    float radius1 = (RADIUS / 2.) * ht;
    float radius2 = RADIUS / 2. + (RADIUS / 2.) * ht;
    
    float alpha1 = 1.;
    float alpha2 = 1. - ht;
    
    //
    // Render
    //
        
    float w1 = 
        smoothstep(radius1 - THICKNESS / 2., radius1, d)
        - smoothstep(radius1, radius1 + THICKNESS / 2., d);

    float w2 = 
        smoothstep(radius2 - THICKNESS / 2., radius2, d)
        - smoothstep(radius2, radius2 + THICKNESS / 2., d);

    float whiteDepth = max(w1 * alpha1, w2 * alpha2) * .5;
    
    return vec4(whiteDepth, whiteDepth, whiteDepth, 1.); // white
}

void main()
{
    float d = length(virtualSpacePosition);

    vec4 grip_circle = make_grip_circle(d);
    vec4 pressure_injection_halo = make_pressure_injection_halo(d, float1);
    
    gl_FragColor =
        grip_circle * is_type(notification_type, 1.0)
        + pressure_injection_halo * is_type(notification_type, 2.0);
}
