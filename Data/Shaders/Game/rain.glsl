###VERTEX-120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec2 inRain; // Position (vec2)

// Outputs
out vec2 uv;

void main()
{
    uv = inRain.xy;
    gl_Position = vec4(inRain.xy, -1.0, 1.0);
}


###FRAGMENT-120

#define in varying

// Inputs
in vec2 uv;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform vec3 paramEffectiveMoonlightColor;
uniform float paramRainAngle;
uniform float paramRainDensity;
uniform float paramTime;
uniform vec2 paramViewportSize;

// Based on https://www.shadertoy.com/view/wsKcWw by evilRyu

float hash(vec2 p)
{
    p = 50.0 * fract(p * 0.3183099);
    return fract(p.x * p.y * (p.x + p.y));
}

float noise(vec2 x)
{
    vec2 p = floor(x);
    vec2 w = fract(x);
    
    float a = hash(p + vec2(0,0));
    float b = hash(p + vec2(1,0));
    float c = hash(p + vec2(0,1));
    float d = hash(p + vec2(1,1));
    
    return -1.0 + 2.0 * (a + (b-a) * w.x + (c-a) * w.y + (a - b - c + d) * w.x * w.y);
}

void main()
{
    //
    // ---------------------------------------------
    //

   vec2 uvScaled = uv * (paramViewportSize.x / 520.0);
    
    #define SPEED .13
    vec2 timeVector = vec2(paramTime * SPEED);
    
    vec2 directionVector = vec2((uvScaled.y + 1.) / 2. * paramRainAngle, 0.);
    
    vec2 st = 256. * (uvScaled * vec2(.5, .01) + timeVector + directionVector);
    
    float alpha = noise(st) * noise(st * 0.773) * 0.485;
    alpha = clamp(alpha, 0.0, 1.0) * step(1. - (.7 + paramRainDensity * .3), alpha);
    vec3 col = 
        vec3(1. - alpha)
        * mix(
            paramEffectiveMoonlightColor * 0.75, 
            vec3(1.), 
            paramEffectiveAmbientLightIntensity);

    //
    // ---------------------------------------------
    //

    // Output to screen
    gl_FragColor = vec4(col, alpha);
} 
