###VERTEX

#version 120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec2 inRain; // Position (vec2)

// Outputs
out vec2 uv;

void main()
{
    uv = (vec2(1.0, 1.0) + inRain.xy) / 2.0;
    gl_Position = vec4(inRain.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

// Inputs
in vec2 uv;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramRainDensity;
uniform float paramTime;

void main()
{
    //
    // ---------------------------------------------
    //
    
    #define RainSpatialDensityX 45.0
    #define RainSpatialDensityY 30.0
    #define RainSpeed 30.0
    #define DropletLength .9
    #define DropletWidth .08
    
    vec2 scaledUV = uv * vec2(RainSpatialDensityX, RainSpatialDensityY);
        
    // Calculate tile X coordinate
    float tileX = floor(scaledUV.x);
    
	// Offset Y based off time, and in two different ways to provide 
    // a sense of depth
    scaledUV.y += paramTime * RainSpeed * (.8 + mod(tileX, 2.) * .2);
                
    // Offset Y randomly based off its tile X coordinate
    scaledUV.y += 407.567 * fract(351.5776456 * tileX);
    
    // Calculate tile Y coordinate
    float tileY = floor(scaledUV.y);

    // Decide whether tile is turned off
    float randOnOff = fract(sin(tileX * 71. + tileY * 7.));
    float onOffThickness = 1. - step(paramRainDensity, randOnOff);

    // Shortcut
    if (onOffThickness == 0.)
        discard;
    
    // Calculate coords within the tile
    vec2 inTile = fract(scaledUV);
    
    // Shuffle tile center X based on its tile coordinates
    float rand = fract(sin(77.7 * tileY + 7.7 * tileX));
    float tileCenterX = .5 + (abs(rand) - .5) * (1. - DropletWidth);
        
    // Distance from center of tile
    float xDistance = abs(tileCenterX - inTile.x);
    float yDistance = abs(.5 - inTile.y);
    	    
    // Thickness of droplet:
    //    > 0.0 only within Width
    //    ...with some tile-specific randomization
    float clampedXDistance = smoothstep(.0, DropletWidth, xDistance);
    float dropletThickness = (1. - clampedXDistance) * smoothstep(1. - DropletLength + rand/2.4, 1., 1. - yDistance);

    //
    // ---------------------------------------------
    //

    float alpha = .4 * smoothstep(.4, 1., dropletThickness);
    vec3 c = vec3(dropletThickness, dropletThickness, dropletThickness) * paramEffectiveAmbientLightIntensity;

    // Output to screen
    gl_FragColor = vec4(c, alpha);
} 
