###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in float inShipPointWater;
in vec4 inShipPointColor;
in vec2 inShipPointAttributeGroup1; // Light, PlaneId

// Outputs        
out float vertexLightIntensity;
out float vertexLightColorMix;
out float vertexWater;
out vec4 vertexCol;

// Params
uniform float paramAmbientLightIntensity;
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLightIntensity = paramAmbientLightIntensity + (1.0 - paramAmbientLightIntensity) * inShipPointAttributeGroup1.x;
    vertexLightColorMix = inShipPointAttributeGroup1.x;
    vertexWater = inShipPointWater;
    vertexCol = inShipPointColor;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, inShipPointAttributeGroup1.y, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in float vertexLightIntensity;
in float vertexLightColorMix;
in float vertexWater;
in vec4 vertexCol;

// Params
uniform float paramWaterContrast;
uniform float paramWaterLevelThreshold;

void main()
{
    // Apply point water
    float colorWetness = min(vertexWater, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast;
    vec3 fragColour = vertexCol.xyz * (1.0 - colorWetness) + vec3(%WET_COLOR_VEC3%) * colorWetness;
    
     // Apply light
    fragColour *= vertexLightIntensity;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexLightColorMix) + vec3(%LAMPLIGHT_COLOR_VEC3%) * vertexLightColorMix;

    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 

