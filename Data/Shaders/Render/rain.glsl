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
    uv = (vec2(1.0, 1.0) + inRain.xy) / 2.0;
    gl_Position = vec4(inRain.xy, -1.0, 1.0);
}


###FRAGMENT-120

#define in varying

// Inputs
in vec2 uv;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramRainAngle;
uniform float paramRainDensity;
uniform float paramTime;
uniform vec2 paramViewportSize;

// Based on https://www.shadertoy.com/view/wsKcWw by evilRyu

float hash1(vec2 p)
{
    p  = 50.0 * fract(p * 0.3183099);
    return fract(p.x * p.y * (p.x + p.y));
}

float noise(vec2 x)
{
    vec2 p = floor(x);
    vec2 w = fract(x);
    vec2 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    
    float a = hash1(p+vec2(0,0));
    float b = hash1(p+vec2(1,0));
    float c = hash1(p+vec2(0,1));
    float d = hash1(p+vec2(1,1));
    
    return -1.0+2.0*(a + (b-a)*u.x + (c-a)*u.y + (a - b - c + d)*u.x*u.y);
}

void main()
{
    //
    // ---------------------------------------------
    //

    vec2 uvScaled = uv * (paramViewportSize.x / 620.0);
    
    #define SPEED .13
    vec2 timeVector = vec2(paramTime * SPEED);
    
    vec2 directionVector = vec2((uvScaled.y + 1.) / 2. * paramRainAngle, 0.);
    
    vec2 st = 256. * (uvScaled * vec2(.5, .01) + timeVector + directionVector);
    float f = noise(st) * noise(st * 0.773) * 1.55;
    
    f = clamp(pow(abs(f), 1.0 / paramRainDensity * 13.0) * 13.0, 0.0, .14);
    
    # define LUMINANCE 1.94 // The higher, the whiter
    vec3 col = f * vec3(LUMINANCE * paramEffectiveAmbientLightIntensity);    
    col = clamp(col, 0.0, 1.0);
    
    float alpha = col.x;
    col = vec3(1.) - col;

    //
    // ---------------------------------------------
    //

    // Output to screen
    gl_FragColor = vec4(col, alpha);
} 
