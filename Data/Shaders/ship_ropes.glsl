###VERTEX

#version 130

// Inputs
in vec2 inShipPointPosition;        
in float inShipPointLight;
in float inShipPointWater;

// Outputs        
out float vertexLight;
out float vertexWater;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = inShipPointLight;
    vertexWater = inShipPointWater;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Inputs from previous shader        
in float vertexLight;
in float vertexWater;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterLevelThreshold;

void main()
{
    vec4 ropeColor = vec4(%ROPE_COLOR_VEC4%);

    // Apply point water
    float colorWetness = min(vertexWater, paramWaterLevelThreshold) * 0.7 / paramWaterLevelThreshold;
    vec3 fragColour = ropeColor.xyz * (1.0 - colorWetness) + vec3(%WET_COLOR_VEC3%) * colorWetness;

    // Apply ambient light
    fragColour *= paramAmbientLightIntensity;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexLight) + vec3(%LAMPLIGHT_COLOR_VEC3%) * vertexLight;
    
    gl_FragColor = vec4(fragColour.xyz, ropeColor.w);
} 
