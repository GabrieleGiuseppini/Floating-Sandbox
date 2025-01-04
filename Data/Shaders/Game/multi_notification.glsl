
// Kept in sync with code
#define BLAST_TOOL_HALO 1.0
#define FIRE_EXTINGUISHER_SPRAY 2.0
#define GRIP_CIRCLE 3.0
#define PRESSURE_INJECTION_HALO 4.0

###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inMultiNotification1;  // Type, WorldPosition (vec2), [FlowMultiplier|Progress] float1 (float)
in vec3 inMultiNotification2; // VirtualSpacePosition (vec2), [PersonalitySeed] float2 (float)

// Outputs
out float notification_type;
out float float1;
out float float2;
out vec2 virtualSpacePosition;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    notification_type = inMultiNotification1.x;
    float1 = inMultiNotification1.w;
    float2 = inMultiNotification2.z;
    virtualSpacePosition = inMultiNotification2.xy;

    gl_Position = paramOrthoMatrix * vec4(inMultiNotification1.yz, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float notification_type;
in float float1; // FlowMultiplier|Progress
in float float2; // PersonalitySeed
in vec2 virtualSpacePosition; // [-1.0, 1.0]

// Parameters
uniform float paramTime;
uniform sampler2D paramNoiseTexture;

#define PI 3.14159265358979323844

float is_type(float notification_type, float value)
{
    return step(value - 0.5, notification_type) * step(notification_type, value + 0.5);
}

mat2 get_rotation_matrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);

    return m;
}


vec4 make_blast_tool_halo(
    float d,    
    float noise)
{
    //
    // Border
    //
        
    #define BORDER_WIDTH 0.05
    float whiteDepth = 
        smoothstep(1.0 - BORDER_WIDTH - BORDER_WIDTH - noise, 1.0 - BORDER_WIDTH, d)
        - smoothstep(1.0 - BORDER_WIDTH, 1.0 - BORDER_WIDTH / 2., d);
    
    whiteDepth *= d * .7;
    
    return vec4(whiteDepth, whiteDepth, whiteDepth, 1.);
}

vec4 make_fire_extinguisher_spray(
    vec2 virtualSpacePosition,
    float d,
    float sprayTime,
    float noise)
{
    // Rotate based on noise sampled via polar coordinates of pixel

    float angle = noise;
    // Magnify rotation amount based on distance from center of screen
    angle *= 1.54 * smoothstep(0.0, 0.6, d);
    
    // Rotate!
    virtualSpacePosition += get_rotation_matrix(angle) * virtualSpacePosition;
    
    //
    // Transform to polar coordinates
    //
    
    // (r, a) (r=[0.0, 1.0], a=[0.0, 1.0 CCW from W])
        
    vec2 ra = vec2(
        d, 
        (atan(virtualSpacePosition.y, virtualSpacePosition.x) / (2.0 * PI) + 0.5));
        
    // Scale radius to better fit in quad
    ra.x *= 1.7;                   

    
    //
    // Randomize radius based on noise and radius
    //
    
    #define VariationRNoiseResolution 1.0
    float variationR = texture2D(
        paramNoiseTexture, 
        ra * vec2(VariationRNoiseResolution / 4.0, VariationRNoiseResolution / 1.0) 
        + vec2(-sprayTime, 0.0)).r;
    
    variationR -= 0.5;

    // Straighten the spray at the center and make full turbulence outside,
    // scaling it at the same time
    variationR *= 0.35 * smoothstep(-0.40, 0.4, ra.x);

    float radius = ra.x + variationR;

    // Focus (compress dynamics)
    float alpha = 1.0 - smoothstep(0.2, 1.4, radius);

    return vec4(alpha);
}

vec4 make_grip_circle(float d)
{
    #define MAX_ALPHA 0.24

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
    // Common to many
    float d = length(virtualSpacePosition);

    // Common to many
    vec2 noiseSampleCoords = vec2(0.0);
    {
        // Blast tool halo

        float theta = atan(virtualSpacePosition.y, virtualSpacePosition.x) / (2.0 * PI);
        noiseSampleCoords += 
            vec2(0.015 * float1 + float2, theta) 
            * is_type(notification_type, BLAST_TOOL_HALO);
    }
    float sprayTime;
    {
        // Fire extinguisher spray

        #define SpraySpeed 0.3
        sprayTime = paramTime * SpraySpeed;    
    
        // Rotate based on noise sampled via polar coordinates of pixel
        
        // (r, a) (r=[0.0, 1.0], a=[0.0, 1.0 CCW from W])
        vec2 angleNoiseRa = vec2(
            d, 
            (atan(virtualSpacePosition.y, virtualSpacePosition.x) / (2.0 * PI) + 0.5));
        
        #define AngleNoiseResolution 1.0        
        noiseSampleCoords += 
            (
                angleNoiseRa * vec2(AngleNoiseResolution, AngleNoiseResolution) 
                + vec2(-0.5 * sprayTime, 0.2* sprayTime)
            )
            * is_type(notification_type, BLAST_TOOL_HALO);
    }
    float noise = texture2D(paramNoiseTexture, noiseSampleCoords).r; // 0.0 -> 1.0

    // Generate frag colors
    vec4 blast_tool_halo = make_blast_tool_halo(d, noise);
    vec4 fire_extinguisher_spray = make_fire_extinguisher_spray(virtualSpacePosition, d, sprayTime, noise);
    vec4 grip_circle = make_grip_circle(d);
    vec4 pressure_injection_halo = make_pressure_injection_halo(d, float1);
    
    // Pick frag color
    gl_FragColor =
        blast_tool_halo * is_type(notification_type, BLAST_TOOL_HALO)
        + fire_extinguisher_spray * is_type(notification_type, FIRE_EXTINGUISHER_SPRAY)
        + grip_circle * is_type(notification_type, GRIP_CIRCLE)
        + pressure_injection_halo * is_type(notification_type, PRESSURE_INJECTION_HALO);
}
