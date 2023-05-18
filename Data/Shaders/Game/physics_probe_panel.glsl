###VERTEX-120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec4 inPhysicsProbePanel1; // Position (vec2), TextureFrameOffset (vec2, from 0.0 to width - inclusive dx)
in vec3 inPhysicsProbePanel2; // XLimits (vec2), Opening (float)

// Outputs
out vec2 vertexCoordinatesNdc;
out vec2 vertexTextureFrameOffset;
out vec2 xLimitsNdc;
out float vertexIsOpening;

void main()
{
    vertexCoordinatesNdc = inPhysicsProbePanel1.xy;
    vertexTextureFrameOffset = inPhysicsProbePanel1.zw; 
    xLimitsNdc = inPhysicsProbePanel2.xy;
    vertexIsOpening = inPhysicsProbePanel2.z;
    gl_Position = vec4(inPhysicsProbePanel1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexCoordinatesNdc;
in vec2 vertexTextureFrameOffset;
in vec2 xLimitsNdc;
in float vertexIsOpening;

// Textures
uniform sampler2D paramGenericLinearTexturesAtlasTexture;
uniform sampler2D paramNoiseTexture;

// Params
uniform vec2 paramAtlasTile1LeftBottomTextureCoordinates;
uniform float paramWidthNdc;

float GetNoise(float y, float seed, float time) // -> (0.0, 1.0)
{
    #define NOISE_RESOLUTION 0.25
    float s1 = texture2D(paramNoiseTexture, vec2(NOISE_RESOLUTION * y + time, seed)).r;
    float s2 = texture2D(paramNoiseTexture, vec2(NOISE_RESOLUTION * y - time, seed)).r;
    
    #define NOISE_THRESHOLD .65
    s1 = (clamp(s1, NOISE_THRESHOLD, 1.) - NOISE_THRESHOLD) * 1. / (1. - NOISE_THRESHOLD);
    s2 = (clamp(s2, NOISE_THRESHOLD, 1.) - NOISE_THRESHOLD) * 1. / (1. - NOISE_THRESHOLD);
    
    return max(s1, s2);
}

void main()
{
    //
    // Flanges
    //
    
    float currentWidthNdc = xLimitsNdc.y - xLimitsNdc.x; // 0.0 -> 0.5
    float midXNdc = xLimitsNdc.x + currentWidthNdc / 2.;
    float widthFraction = currentWidthNdc / paramWidthNdc;
    
    float noise = GetNoise(
        vertexCoordinatesNdc.y / (.4 * paramWidthNdc * .5), 
        .2 * step(midXNdc, vertexCoordinatesNdc.x), 
        widthFraction / 5.);
    
    float flangeLengthNdc = (0.25 + noise * .4) * paramWidthNdc / 2.;
    
    // Flatten flange when panel is too small or almost fully open
    flangeLengthNdc *= (smoothstep(.0, .46, widthFraction) - smoothstep(.54, 1., widthFraction));
    
    // Make flanges close to x Limits, either external or internal
    // depending on whether we're closing or opening
    float leftFlange = 
        vertexIsOpening * smoothstep(0.0, flangeLengthNdc, vertexCoordinatesNdc.x - xLimitsNdc.x)
        + (1. - vertexIsOpening) * (1. - smoothstep(0.0, flangeLengthNdc, xLimitsNdc.x - vertexCoordinatesNdc.x));
    float rightFlange = 
        vertexIsOpening * smoothstep(0.0, flangeLengthNdc, xLimitsNdc.y - vertexCoordinatesNdc.x)
        + (1. - vertexIsOpening) * (1. - smoothstep(0.0, flangeLengthNdc, vertexCoordinatesNdc.x - xLimitsNdc.y));
        
    // Pick L or R flange depending on where this pixel is wrt mid
    float isLeft = step(vertexCoordinatesNdc.x, midXNdc);
    float panelDepth = 
        isLeft * leftFlange
        + (1. - isLeft) * rightFlange;
                
    float inPanelQuad = step(xLimitsNdc.x, vertexCoordinatesNdc.x) * step(vertexCoordinatesNdc.x, xLimitsNdc.y);
                     
    //
    // Texture
    //
    
    vec4 cTexture = texture2D(paramGenericLinearTexturesAtlasTexture, paramAtlasTile1LeftBottomTextureCoordinates + vertexTextureFrameOffset);
    
           
    //
    // Final color
    //
            
    vec4 color = vec4(cTexture.xyz, panelDepth * cTexture.w);
    
    // Add white contour
    float panelBorderDepth = max(
        abs(inPanelQuad - panelDepth),
        1.0 - smoothstep(0.0, 0.005, min(abs(vertexCoordinatesNdc.x - xLimitsNdc.x), abs(vertexCoordinatesNdc.x - xLimitsNdc.y))));
    gl_FragColor = mix(
        color,
        vec4(1.),
        panelBorderDepth);
} 
