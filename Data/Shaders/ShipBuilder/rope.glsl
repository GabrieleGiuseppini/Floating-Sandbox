###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inRope1; // Vertex position (ship space)
in vec4 inRope2; // Color

// Outputs
out vec4 ropeColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    ropeColor = inRope2;
    gl_Position = paramOrthoMatrix * vec4(inRope1.xy, 0.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec4 ropeColor;

// Params
uniform float paramOpacity;

void main()
{
    gl_FragColor = vec4(ropeColor.xyz, ropeColor.w * paramOpacity);
} 
