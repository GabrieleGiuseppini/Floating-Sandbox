###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in float inShipPointLight;
in float inShipPointWater;
in vec4 inShipPointColor;
in float inShipPointPlaneId;

// Outputs        
out float vertexLight;
out float vertexWater;
out vec4 vertexCol;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = inShipPointLight;
    vertexWater = inShipPointWater;
    vertexCol = inShipPointColor;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, inShipPointPlaneId, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in float vertexLight;
in float vertexWater;
in vec4 vertexCol;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterContrast;
uniform float paramWaterLevelThreshold;

void main()
{
    // Apply point water
    float colorWetness = min(vertexWater, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast;
    vec3 fragColour = vertexCol.xyz * (1.0 - colorWetness) + vec3(%WET_COLOR_VEC3%) * colorWetness;

     // Apply ambient light
    fragColour *= paramAmbientLightIntensity;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexLight) + vec3(%LAMPLIGHT_COLOR_VEC3%) * vertexLight;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 

