###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec2 inShipPointPosition;
in float inShipPointLight;
in float inShipPointWater;
in vec2 inShipPointTextureCoordinates;

// Outputs        
out float vertexLight;
out float vertexWater;
out vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = inShipPointLight;
    vertexWater = inShipPointWater;
    vertexTextureCoords = inShipPointTextureCoordinates;

    gl_Position = paramOrthoMatrix * vec4(inShipPointPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in float vertexLight;
in float vertexWater;
in vec2 vertexTextureCoords;

// Input texture
uniform sampler2D sharedSpringTexture;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterContrast;
uniform float paramWaterLevelThreshold;

void main()
{
    vec4 vertexCol = texture2D(sharedSpringTexture, vertexTextureCoords);

    // Apply point water
    float colorWetness = min(vertexWater, paramWaterLevelThreshold) / paramWaterLevelThreshold * paramWaterContrast;
    vec4 fragColour = vertexCol * (1.0 - colorWetness) + vec4(%WET_COLOR_VEC4%) * colorWetness;

    // Apply ambient light
    fragColour *= paramAmbientLightIntensity;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexLight) + vec4(%LAMPLIGHT_COLOR_VEC4%) * vertexLight;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 
