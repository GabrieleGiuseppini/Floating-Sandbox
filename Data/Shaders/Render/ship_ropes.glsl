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
out vec3 vertexPackedParams; // LightIntensity, LightColorMix, ColorWetness
out vec4 vertexCol;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterContrast;
uniform float paramWaterLevelThreshold;
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexPackedParams = vec3(
        paramAmbientLightIntensity + (1.0 - paramAmbientLightIntensity) * inShipPointAttributeGroup1.x, // LightIntensity
        inShipPointAttributeGroup1.x, // LightColorMix        
        min(inShipPointWater, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast); // ColorWetness

    vertexCol = inShipPointColor;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, inShipPointAttributeGroup1.y, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in vec3 vertexPackedParams; // LightIntensity, LightColorMix, ColorWetness
in vec4 vertexCol;

void main()
{
    // Apply point water
    vec3 fragColour = vertexCol.xyz * (1.0 - vertexPackedParams.z) + vec3(%WET_COLOR_VEC3%) * vertexPackedParams.z;

     // Apply light
    fragColour *= vertexPackedParams.x;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexPackedParams.y) + vec3(%LAMPLIGHT_COLOR_VEC3%) * vertexPackedParams.y;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 
