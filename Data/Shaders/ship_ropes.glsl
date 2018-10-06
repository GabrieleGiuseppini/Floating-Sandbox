###VERTEX

// Inputs
attribute vec2 inputPos;        
attribute float inputLight;
attribute float inputWater;

// Outputs        
varying float vertexLight;
varying float vertexWater;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = inputLight;
    vertexWater = inputWater;

    gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
}

###FRAGMENT

// Inputs from previous shader        
varying float vertexLight;
varying float vertexWater;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterLevelThreshold;

void main()
{
    // Apply point water
    float colorWetness = min(vertexWater, paramWaterLevelThreshold) * 0.7 / paramWaterLevelThreshold;
    vec3 fragColour = vec3(%ROPE_COLOR_VEC3%) * (1.0 - colorWetness) + vec3(%WET_COLOR_VEC3%) * colorWetness;

    // Apply ambient light
    fragColour *= paramAmbientLightIntensity;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexLight) + vec3(%LAMPLIGHT_COLOR_VEC3%) * vertexLight;
    
    gl_FragColor = vec4(fragColour.xyz, 1.0);
} 
