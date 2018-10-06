###VERTEX

// Inputs
attribute vec2 inputPos;        
attribute float inputLight;
attribute float inputWater;
attribute vec2 inputTextureCoords;

// Outputs        
varying float vertexLight;
varying float vertexWater;
varying vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexLight = inputLight;
    vertexWater = inputWater;
    vertexTextureCoords = inputTextureCoords;

    gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
}

###FRAGMENT

// Inputs from previous shader        
varying float vertexLight;
varying float vertexWater;
varying vec2 vertexTextureCoords;

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
