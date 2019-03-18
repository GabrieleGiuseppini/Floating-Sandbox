###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in vec3 inShipPointAttributeGroup1; // Light, Water, PlaneId
in vec2 inShipPointTextureCoordinates;

// Outputs        
out float vertexLightIntensity;
out float vertexLightColorMix;
out float vertexColorWetness;
out vec2 vertexTextureCoords;

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
    vertexTextureCoords = inShipPointTextureCoordinates;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, inShipPointAttributeGroup1.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in float vertexLightIntensity;
in float vertexLightColorMix;
in float vertexColorWetness;
in vec2 vertexTextureCoords;

// Input texture
uniform sampler2D paramSharedTexture;

void main()
{
    vec4 vertexCol = texture2D(paramSharedTexture, vertexTextureCoords);

    // Discard transparent pixels, so that ropes (which are drawn temporally after
    // this shader but Z-ally behind) are not occluded by transparent triangles
    if (vertexCol.w < 0.2)
        discard;

    // Apply point water
    vec4 fragColour = vertexCol * (1.0 - vertexColorWetness) + vec4(%WET_COLOR_VEC4%) * vertexColorWetness;

    // Apply light
    fragColour *= vertexLightIntensity;

    // Apply point light color
    fragColour = fragColour * (1.0 - vertexLightColorMix) + vec4(%LAMPLIGHT_COLOR_VEC4%) * vertexLightColorMix;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 
