###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inShipPointAttributeGroup1; // Position, TextureCoordinates
in vec4 inShipPointAttributeGroup2; // Light, Water, PlaneId, Decay
in float inShipPointTemperature; // Temperature

// Outputs        
out float vertexTemperature;
out vec2 vertexTextureCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{            
    vertexTemperature = inShipPointTemperature;
    vertexTextureCoords = inShipPointAttributeGroup1.zw;

    gl_Position = paramOrthoMatrix * vec4(inShipPointAttributeGroup1.xy, inShipPointAttributeGroup2.z, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in float vertexTemperature;
in vec2 vertexTextureCoords;

// Params
uniform float paramAmbientLightIntensity;

// Input texture
uniform sampler2D paramSharedTexture;

void main()
{
    vec4 vertexCol = texture2D(paramSharedTexture, vertexTextureCoords);

    // Discard transparent pixels, so that ropes (which are drawn temporally after
    // this shader but Z-ally behind) are not occluded by transparent triangles
    if (vertexCol.w < 0.2)
        discard;

    // Select color based on temperature, between red and yellow
    vec3 heatColor = mix(vec3(%HEAT_RED_COLOR_VEC3%), vec3(%HEAT_YELLOW_COLOR_VEC3%), 1.0 - exp(-0.0008 * vertexTemperature));

    // Apply ambient light
    heatColor *= paramAmbientLightIntensity;

    // Create fragment color using own transparency and texture's transparency
    gl_FragColor = vec4(heatColor, 1.0); // 0.5 * vertexCol.w);
} 
