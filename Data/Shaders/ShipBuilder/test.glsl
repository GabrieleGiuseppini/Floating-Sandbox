###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec2 inTest; // Position (vec2)

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inTest, -1.0, 1.0);   
}

###FRAGMENT-120

#define in varying

void main()
{
    float depth = smoothstep(0.0, 1.0, mod(gl_FragCoord.x, 10.0));
    gl_FragColor = vec4(depth, depth, depth, 1.0);
} 
