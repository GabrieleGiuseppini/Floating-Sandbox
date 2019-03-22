###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inShipPointAttributeGroup1; // Position, TextureCoordinates
in vec4 inShipPointAttributeGroup2; // Light, Water, PlaneId, Decay
in vec4 inShipPointColor;

// Outputs        
out float vertexLight;
out float vertexWater;
out vec4 vertexCol;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = inShipPointAttributeGroup2.x;
    vertexWater = inShipPointAttributeGroup2.y;
    vertexCol = inShipPointColor;

    gl_Position = paramOrthoMatrix * vec4(inShipPointAttributeGroup1.xy, inShipPointAttributeGroup2.z, 1.0);
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
    float vertexColorWetness = min(vertexWater, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast;
    vec4 fragColour = vertexCol * (1.0 - vertexColorWetness) + vec4(%WET_COLOR_VEC4%) * vertexColorWetness;

    // Complement missing ambient light with point's light
    float totalLightIntensity = paramAmbientLightIntensity + (1.0 - paramAmbientLightIntensity) * vertexLight;

    // Apply light
    fragColour *= totalLightIntensity;

    // Apply point light color
    fragColour = fragColour * (1.0 - vertexLight) + vec4(%LAMPLIGHT_COLOR_VEC4%) * vertexLight;

    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 

