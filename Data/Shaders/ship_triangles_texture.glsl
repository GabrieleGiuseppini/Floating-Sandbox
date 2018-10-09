###VERTEX

#version 130

// Inputs
in vec2 shipPointPosition;
in float shipPointLight;
in float shipPointWater;
in vec2 shipPointTextureCoordinates;

// Outputs        
out float vertexLight;
out float vertexWater;
out vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = shipPointLight;
    vertexWater = shipPointWater;
    vertexTextureCoords = shipPointTextureCoordinates;

    gl_Position = paramOrthoMatrix * vec4(shipPointPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Inputs from previous shader        
in float vertexLight;
in float vertexWater;
in vec2 vertexTextureCoords;

// Input texture
uniform sampler2D inputTexture;

// Params
uniform float paramAmbientLightIntensity;
uniform float paramWaterLevelThreshold;

void main()
{
    vec4 vertexCol = texture2D(inputTexture, vertexTextureCoords);

    // Apply point water
    float colorWetness = min(vertexWater, paramWaterLevelThreshold) * 0.7 / paramWaterLevelThreshold;
    vec4 fragColour = vertexCol * (1.0 - colorWetness) + vec4(%WET_COLOR_VEC4%) * colorWetness;

    // Apply ambient light
    fragColour *= paramAmbientLightIntensity;

    // Apply point light
    fragColour = fragColour * (1.0 - vertexLight) + vec4(%LAMPLIGHT_COLOR_VEC4%) * vertexLight;
    
    gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
} 
