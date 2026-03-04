
// Kept in sync with code
#define POWER_METER 1.0

###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inMultiNotification_NdcCoords1;  // Type (float), WorldPosition (vec2), [BorderVirtualSpaceWidth] float1 (float)
in vec3 inMultiNotification_NdcCoords2; // [BorderVirtualSpaceHeight] float2 (float), [VirtualSpacePosition -1..1] auxPosition (vec2)
in vec4 inMultiNotification_NdcCoords3; // [StartColor] color1 (vec4)
in vec4 inMultiNotification_NdcCoords4; // [EndColor] color2 (vec4)

// Outputs
out float notification_type;
out float float1;
out float float2;
out vec2 auxPosition;
out vec4 color1;
out vec4 color2;

void main()
{
    notification_type = inMultiNotification_NdcCoords1.x;
    float1 = inMultiNotification_NdcCoords1.w;
    float2 = inMultiNotification_NdcCoords2.x;
    auxPosition = inMultiNotification_NdcCoords2.yz;
    color1 = inMultiNotification_NdcCoords3;
    color2 = inMultiNotification_NdcCoords4;

    gl_Position = vec4(inMultiNotification_NdcCoords1.yz, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float notification_type;
in float float1; // BorderVirtualSpaceWidth
in float float2; // BorderVirtualSpaceHeight
in vec2 auxPosition; // VirtualSpacePosition -1..1
in vec4 color1; // StartColor
in vec4 color2; // EndColor

float is_type(float notification_type, float value)
{
    return step(value - 0.5, notification_type) * step(notification_type, value + 0.5);
}

/////////////////////////

vec4 make_power_meter()
{
    #define ANTI_ALIAS_BORDER_FRACTION 0.2
    float absx = abs(auxPosition.x);
    float borderDepthX = smoothstep(1.0 - float1, 1.0, absx);
    float absy = abs(auxPosition.y);
    float borderDepthY = smoothstep(1.0 - float2, 1.0, absy);

    vec4 color = mix(color1, color2, (1. + auxPosition.y) / 2.);

    return vec4(
        color.rgb * (1.0 - max(borderDepthX, borderDepthY)),
        color.a);
}

void main()
{
    // Generate frag colors
    vec4 power_meter = make_power_meter();

    // Pick frag color
    gl_FragColor =
        power_meter * is_type(notification_type, POWER_METER)
        ;
}
