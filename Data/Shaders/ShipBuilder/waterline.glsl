###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inWaterline1; // Vertex position (ship space), Center coords (ship space)
in vec2 inWaterline2; // Direction

// Outputs (ship space)
out vec2 vertexCoordinates;
out vec2 centerCoordinates;
out vec2 direction;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexCoordinates = inWaterline1.xy;
    centerCoordinates = inWaterline1.zw;
    direction = inWaterline2;

    gl_Position = paramOrthoMatrix * vec4(inWaterline1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader (ship space)
in vec2 vertexCoordinates;
in vec2 centerCoordinates;
in vec2 direction;

void main()
{
    // TODO: make it a param
    vec4 waterColor = vec4(0.49, 0.89, 0.93, 0.6);

    // Calculate alignment towards direction
    float alignment = dot(vertexCoordinates - centerCoordinates, direction);

    gl_FragColor = mix(
        vec4(0.0),
        waterColor,
        step(0.0, alignment));
} 
