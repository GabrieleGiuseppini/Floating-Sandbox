###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inWaterline1; // Vertex position (ship space), Center coords (ship space)
in vec2 inWaterline2; // Direction

// Outputs
out vec2 vertexCoordinates;
out vec2 centerCoordinates;
out vec2 direction;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vec4 position = paramOrthoMatrix * vec4(inWaterline1.xy, 0.0, 1.0);

    vertexCoordinates = position.xy;
    centerCoordinates = (paramOrthoMatrix * vec4(inWaterline1.zw, 0.0, 1.0)).xy;
    direction = inWaterline2;
    
    gl_Position = position;
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexCoordinates;
in vec2 centerCoordinates;
in vec2 direction;

void main()
{
    // TODO: make it a param
    vec4 waterColor = vec4(0.49, 0.89, 0.93, 1.0);

    // Calculate alignment towards direction
    float alignment = dot(vertexCoordinates - centerCoordinates, direction);

    gl_FragColor = mix(
        vec4(0.0),
        waterColor,
        step(0.0, alignment));
} 
