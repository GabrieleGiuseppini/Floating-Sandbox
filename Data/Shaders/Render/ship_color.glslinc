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
out float vertexDecay;
out vec4 vertexCol;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = inShipPointAttributeGroup2.x;
    vertexWater = inShipPointAttributeGroup2.y;
    vertexDecay = inShipPointAttributeGroup2.w;
    vertexCol = inShipPointColor;

    gl_Position = paramOrthoMatrix * vec4(inShipPointAttributeGroup1.xy, inShipPointAttributeGroup2.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in float vertexLight;
in float vertexWater;
in float vertexDecay;
in vec4 vertexCol;

// Params
uniform float paramAmbientLightIntensity;
uniform vec4 paramWaterColor;
uniform float paramWaterContrast;
uniform float paramWaterLevelThreshold;

void main()
{
    // Apply decay
    float originalLightness = (vertexCol.x + vertexCol.y + vertexCol.z) / 3.0;
    vec4 blendColor = vec4(%ROT_GREEN_COLOR%) * (1.0 - originalLightness) + vec4(%ROT_BROWN_COLOR%) * originalLightness;
    vec4 fragColour = vertexCol * vertexDecay + blendColor * (1.0 - vertexDecay);

    // Apply point water
    float vertexColorWetness = min(vertexWater, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast;
    fragColour = fragColour * (1.0 - vertexColorWetness) + paramWaterColor * vertexColorWetness;

    // Complement missing ambient light with point's light
    float totalLightIntensity = paramAmbientLightIntensity + (1.0 - paramAmbientLightIntensity) * vertexLight;

    // Apply light
    fragColour *= totalLightIntensity;

    // Apply point light color
    fragColour = fragColour * (1.0 - vertexLight) + vec4(%LAMPLIGHT_COLOR_VEC4%) * vertexLight;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 

