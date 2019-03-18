###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in vec3 inShipPointAttributeGroup1; // Light, Water, PlaneId
in vec4 inShipPointColor;

// Outputs        
out float vertexLightIntensity;
out float vertexLightColorMix;
out float vertexColorWetness;
out vec4 vertexCol;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterContrast;
uniform float paramWaterLevelThreshold;
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLightIntensity = paramAmbientLightIntensity + (1.0 - paramAmbientLightIntensity) * inShipPointAttributeGroup1.x;
    vertexLightColorMix = inShipPointAttributeGroup1.x;
    vertexColorWetness = min(inShipPointAttributeGroup1.y, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast;
    vertexCol = inShipPointColor;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, inShipPointAttributeGroup1.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in float vertexLightIntensity;
in float vertexLightColorMix;
in float vertexColorWetness;
in vec4 vertexCol;

void main()
{
    // Apply point water
    vec3 fragColour = vertexCol.xyz * (1.0 - vertexColorWetness) + vec3(%WET_COLOR_VEC3%) * vertexColorWetness;

     // Apply light
    fragColour *= vertexLightIntensity;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexLightColorMix) + vec3(%LAMPLIGHT_COLOR_VEC3%) * vertexLightColorMix;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 
