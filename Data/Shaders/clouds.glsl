###VERTEX

#version 130

//
// Clouds's inputs are directly in NDC coordinates
//

// Inputs
in vec2 inSharedPosition;
in vec2 inSharedTextureCoordinates;

// Outputs
out vec2 texturePos;

void main()
{
    gl_Position = vec4(inSharedPosition.xy, -1.0, 1.0);
    texturePos = inSharedTextureCoordinates;
}

###FRAGMENT

#version 130

// Inputs from previous shader
in vec2 texturePos;

// The texture
uniform sampler2D inputTexture;

// Parameters        
uniform float paramAmbientLightIntensity;

void main()
{
    gl_FragColor = texture2D(inputTexture, texturePos) * paramAmbientLightIntensity;
} 
