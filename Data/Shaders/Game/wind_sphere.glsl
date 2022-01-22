###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inWindSphere1;  // VertexPosition (world), CenterPosition (world)
in vec4 inWindSphere2;  // PreFrontRadius (world), PreFrontIntensity, MainFrontRadius (world), MainFrontIntensity
in float inWindSphere3; // ZeroFrontRadius (world)

// Outputs
out vec2 vertexPositionWorld;
out vec2 centerPositionWorld;
out float preFrontRadiusWorld;
out float preFrontIntensity;
out float mainFrontRadiusWorld;
out float mainFrontIntensity;
out float zeroFrontRadiusWorld;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexPositionWorld = inWindSphere1.xy;
    centerPositionWorld = inWindSphere1.zw;
    preFrontRadiusWorld = inWindSphere2.x;
    preFrontIntensity = inWindSphere2.y;
    mainFrontRadiusWorld = inWindSphere2.z;
    mainFrontIntensity = inWindSphere2.w;
    zeroFrontRadiusWorld = inWindSphere3;

    gl_Position = paramOrthoMatrix * vec4(vertexPositionWorld, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in vec2 vertexPositionWorld;
in vec2 centerPositionWorld;
in float preFrontRadiusWorld;
in float preFrontIntensity;
in float mainFrontRadiusWorld;
in float mainFrontIntensity;
in float zeroFrontRadiusWorld;

// Texture
uniform sampler2D paramNoiseTexture2;

// Parameters
uniform float paramTime;

float GetNoise(vec2 uv) // -> (0.0, 1.0)
{
    return texture2D(paramNoiseTexture2, uv).r;
}

void main()
{
    //
    // 1. Sphere inclusion check
    //

    vec2 radiusArcWorld = vertexPositionWorld - centerPositionWorld;
    
    float pointRadiusWorld = length(radiusArcWorld);

    float isInSphere = 
        (1.0 - step(preFrontRadiusWorld, pointRadiusWorld)) * preFrontIntensity * .5
        * step(mainFrontRadiusWorld, pointRadiusWorld)
        +
        (1.0 - step(mainFrontRadiusWorld, pointRadiusWorld)) * mainFrontIntensity * .5;
        
    isInSphere *= step(zeroFrontRadiusWorld, pointRadiusWorld);
            
    //
    // 2. Wave inclusion check
    //
    
    #define RadialFactor 1. / 200.
    
    // (r, a) (r=[0.0, +INF], a=[0.0, 1.0 CCW from W])
    vec2 ra = vec2(
        length(radiusArcWorld), 
        atan(radiusArcWorld.y, radiusArcWorld.x) / (2.0 * 3.14159265358979323844) + 0.5);
        
    float isInWave = GetNoise(vec2(ra.x * RadialFactor, ra.y) + vec2(-paramTime * .8, 0.));
    
    
    //
    // 3. Combine
    //    
    
    float whiteDepth = isInSphere * isInWave;
        
    // TODOTEST
    //vec4 color = vec4(whiteDepth, whiteDepth, whiteDepth, whiteDepth);
    vec4 color = vec4(1., 1., 1., whiteDepth);
    
    ///////

    gl_FragColor = color;
}
