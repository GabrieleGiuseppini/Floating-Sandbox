###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in float inShipPointLight;
in float inShipPointWater;
in vec2 inShipPointTextureCoordinates;
in float inShipPointPlaneId;

// Outputs        
out vec3 vertexPackedParams; // LightIntensity, LightColorMix, ColorWetness
out vec2 vertexTextureCoords;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterContrast;
uniform float paramWaterLevelThreshold;
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexPackedParams = vec3(
        paramAmbientLightIntensity + (1.0 - paramAmbientLightIntensity) * inShipPointLight, // LightIntensity
        inShipPointLight, // LightColorMix        
        min(inShipPointWater, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast); // ColorWetness

    vertexTextureCoords = inShipPointTextureCoordinates;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, inShipPointPlaneId, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in vec3 vertexPackedParams; // LightIntensity, LightColorMix, ColorWetness
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
    vec4 fragColour = vertexCol * (1.0 - vertexPackedParams.z) + vec4(%WET_COLOR_VEC4%) * vertexPackedParams.z;

    // Apply light
    fragColour *= vertexPackedParams.x;

    // Apply point light color
    fragColour = fragColour * (1.0 - vertexPackedParams.y) + vec4(%LAMPLIGHT_COLOR_VEC4%) * vertexPackedParams.y;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 
