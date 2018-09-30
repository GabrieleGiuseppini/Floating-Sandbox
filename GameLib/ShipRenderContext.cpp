/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipRenderContext.h"

#include "GameException.h"
#include "GameMath.h"

ShipRenderContext::ShipRenderContext(    
    std::optional<ImageData> texture,
    vec3f const & ropeColour,
    TextureRenderManager const & textureRenderManager,
    float const(&orthoMatrix)[4][4],
    float visibleWorldHeight,
    float visibleWorldWidth,
    float canvasToVisibleWorldHeightRatio,
    float ambientLightIntensity,
    float waterLevelOfDetail)
    : mTextureRenderManager(textureRenderManager)
    , mCanvasToVisibleWorldHeightRatio(0)
    , mAmbientLightIntensity(0.0f) // Set later
    , mWaterLevelThreshold(0.0f)
    // Points
    , mPointCount(0u)
    , mPointPositionVBO()
    , mPointLightVBO()
    , mPointWaterVBO()
    , mPointColorVBO()
    , mPointElementTextureCoordinatesVBO()
    // Elements
    , mElementColorShaderProgram()
    , mElementColorShaderOrthoMatrixParameter(0)
    , mElementColorShaderAmbientLightIntensityParameter(0)
    , mElementRopeShaderProgram()
    , mElementRopeShaderOrthoMatrixParameter(0)
    , mElementRopeShaderAmbientLightIntensityParameter(0)
    , mElementTextureShaderProgram()
    , mElementTextureShaderOrthoMatrixParameter(0)
    , mElementTextureShaderAmbientLightIntensityParameter(0)
    , mElementStressedSpringShaderProgram(0)
    , mElementStressedSpringShaderOrthoMatrixParameter(0)
    , mElementPinnedPointShaderProgram(0)
    , mElementPinnedPointShaderOrthoMatrixParameter(0)
    , mElementPinnedPointShaderAmbientLightIntensityParameter(0)
    , mElementBombShaderProgram(0)
    , mElementBombShaderOrthoMatrixParameter(0)
    , mElementBombShaderAmbientLightIntensityParameter(0)
    , mConnectedComponents()
    , mPinnedPointElementBuffer()
    , mPinnedPointVBO()
    , mBombElementBuffer()
    , mBombVBO()
    // Vectors
    , mVectorArrowPointPositionBuffer()
    , mVectorArrowPointPositionVBO()
    // Textures
    , mElementTexture()
    , mElementStressedSpringTexture()
    // Lamps
    , mLampBuffers()
{
    GLuint tmpGLuint;
    GLenum glError;

    // Clear errors
    glError = glGetError();


    //
    // Create point VBOs
    //

    GLuint pointVBOs[5];
    glGenBuffers(5, pointVBOs);
    mPointPositionVBO = pointVBOs[0];
    mPointLightVBO = pointVBOs[1];
    mPointWaterVBO = pointVBOs[2];
    mPointColorVBO = pointVBOs[3];
    mPointElementTextureCoordinatesVBO = pointVBOs[4];
    


    //
    // Create color elements program
    //

    mElementColorShaderProgram = glCreateProgram();

    char const * elementColorVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;        
        attribute float inputLight;
        attribute float inputWater;
        attribute vec3 inputCol;

        // Outputs        
        varying float vertexLight;
        varying float vertexWater;
        varying vec3 vertexCol;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {            
            vertexLight = inputLight;
            vertexWater = inputWater;
            vertexCol = inputCol;

            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(elementColorVertexShaderSource, GL_VERTEX_SHADER, mElementColorShaderProgram);

    char const * elementColorFragmentShaderSource = R"(

        // Inputs from previous shader        
        varying float vertexLight;
        varying float vertexWater;
        varying vec3 vertexCol;

        // Params
        uniform float paramAmbientLightIntensity;
        uniform float paramWaterLevelThreshold;

        // Constants
        vec3 lightColour = vec3(1.0, 1.0, 0.25);
        vec3 wetColour = vec3(0.0, 0.0, 0.8);

        void main()
        {
            float colorWetness = min(vertexWater, paramWaterLevelThreshold) * 0.7 / paramWaterLevelThreshold;
            vec3 fragColour = vertexCol * (1.0 - colorWetness) + wetColour * colorWetness;

            fragColour *= paramAmbientLightIntensity;
            fragColour = fragColour * (1.0 - vertexLight) + lightColour * vertexLight;
            
            gl_FragColor = vec4(fragColour.xyz, 1.0);
        } 
    )";

    GameOpenGL::CompileShader(elementColorFragmentShaderSource, GL_FRAGMENT_SHADER, mElementColorShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mElementColorShaderProgram, PointPosVertexAttribute, "inputPos");
    glBindAttribLocation(*mElementColorShaderProgram, PointLightVertexAttribute, "inputLight");
    glBindAttribLocation(*mElementColorShaderProgram, PointWaterVertexAttribute, "inputWater");
    glBindAttribLocation(*mElementColorShaderProgram, PointColorVertexAttribute, "inputCol");

    // Link
    GameOpenGL::LinkShaderProgram(mElementColorShaderProgram, "Ship Color Elements");

    // Get uniform locations
    mElementColorShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mElementColorShaderProgram, "paramOrthoMatrix");
    mElementColorShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mElementColorShaderProgram, "paramAmbientLightIntensity");
    mElementColorShaderWaterLevelThresholdParameter = GameOpenGL::GetParameterLocation(mElementColorShaderProgram, "paramWaterLevelThreshold");


    //
    // Create rope elements program
    //

    mElementRopeShaderProgram = glCreateProgram();

    char const * elementRopeVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;        
        attribute float inputLight;
        attribute float inputWater;

        // Outputs        
        varying float vertexLight;
        varying float vertexWater;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {            
            vertexLight = inputLight;
            vertexWater = inputWater;

            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(elementRopeVertexShaderSource, GL_VERTEX_SHADER, mElementRopeShaderProgram);

    char const * elementRopeFragmentShaderSource = R"(

        // Inputs from previous shader        
        varying float vertexLight;
        varying float vertexWater;

        // Params
        uniform vec3 paramRopeColour;
        uniform float paramAmbientLightIntensity;
        uniform float paramWaterLevelThreshold;

        // Constants
        vec3 lightColour = vec3(1.0, 1.0, 0.25);
        vec3 wetColour = vec3(0.0, 0.0, 0.8);

        void main()
        {
            vec3 vertexCol = paramRopeColour * paramAmbientLightIntensity;

            // Apply point water
            float colorWetness = min(vertexWater, paramWaterLevelThreshold) * 0.7 / paramWaterLevelThreshold;
            vec3 fragColour = vertexCol * (1.0 - colorWetness) + wetColour * colorWetness;

            // Apply ambient light
            fragColour *= paramAmbientLightIntensity;

            // Apply point light
            fragColour = fragColour * (1.0 - vertexLight) + lightColour * vertexLight;
            
            gl_FragColor = vec4(fragColour.xyz, 1.0);
        } 
    )";

    GameOpenGL::CompileShader(elementRopeFragmentShaderSource, GL_FRAGMENT_SHADER, mElementRopeShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mElementRopeShaderProgram, PointPosVertexAttribute, "inputPos");
    glBindAttribLocation(*mElementRopeShaderProgram, PointLightVertexAttribute, "inputLight");
    glBindAttribLocation(*mElementRopeShaderProgram, PointWaterVertexAttribute, "inputWater");

    // Link
    GameOpenGL::LinkShaderProgram(mElementRopeShaderProgram, "Ship Rope Elements");

    // Get uniform locations
    mElementRopeShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mElementRopeShaderProgram, "paramOrthoMatrix");
    GLint ropeShaderRopeColourParameter = GameOpenGL::GetParameterLocation(mElementRopeShaderProgram, "paramRopeColour");
    mElementRopeShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mElementRopeShaderProgram, "paramAmbientLightIntensity");
    mElementRopeShaderWaterLevelThresholdParameter = GameOpenGL::GetParameterLocation(mElementRopeShaderProgram, "paramWaterLevelThreshold");

    // Set hardcoded parameters
    glUseProgram(*mElementRopeShaderProgram);
    glUniform3f(
        ropeShaderRopeColourParameter,
        ropeColour.x,
        ropeColour.y,
        ropeColour.z);
    glUseProgram(0);



    //
    // Create texture elements program
    //

    mElementTextureShaderProgram = glCreateProgram();

    char const * elementTextureVertexShaderSource = R"(

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
    )";

    GameOpenGL::CompileShader(elementTextureVertexShaderSource, GL_VERTEX_SHADER, mElementTextureShaderProgram);

    char const * elementTextureFragmentShaderSource = R"(

        // Inputs from previous shader        
        varying float vertexLight;
        varying float vertexWater;
        varying vec2 vertexTextureCoords;

        // Input texture
        uniform sampler2D inputTexture;

        // Params
        uniform float paramAmbientLightIntensity;
        uniform float paramWaterLevelThreshold;

        // Constants
        vec4 lightColour = vec4(1.0, 1.0, 0.25, 1.0);
        vec4 wetColour = vec4(0.0, 0.0, 0.8, 1.0);

        void main()
        {
            vec4 vertexCol = texture2D(inputTexture, vertexTextureCoords);

            // Apply point water
            float colorWetness = min(vertexWater, paramWaterLevelThreshold) * 0.7 / paramWaterLevelThreshold;
            vec4 fragColour = vertexCol * (1.0 - colorWetness) + wetColour * colorWetness;

            // Apply ambient light
            fragColour *= paramAmbientLightIntensity;

            // Apply point light
            fragColour = fragColour * (1.0 - vertexLight) + lightColour * vertexLight;
            
            gl_FragColor = vec4(fragColour.xyz, vertexCol.w);
        } 
    )";

    GameOpenGL::CompileShader(elementTextureFragmentShaderSource, GL_FRAGMENT_SHADER, mElementTextureShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mElementTextureShaderProgram, PointPosVertexAttribute, "inputPos");
    glBindAttribLocation(*mElementTextureShaderProgram, PointLightVertexAttribute, "inputLight");
    glBindAttribLocation(*mElementTextureShaderProgram, PointWaterVertexAttribute, "inputWater");
    glBindAttribLocation(*mElementTextureShaderProgram, PointTextureCoordinatesVertexAttribute, "inputTextureCoords");

    // Link
    GameOpenGL::LinkShaderProgram(mElementTextureShaderProgram, "Ship Texture Elements");

    // Get uniform locations
    mElementTextureShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mElementTextureShaderProgram, "paramOrthoMatrix");
    mElementTextureShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mElementTextureShaderProgram, "paramAmbientLightIntensity");
    mElementTextureShaderWaterLevelThresholdParameter = GameOpenGL::GetParameterLocation(mElementTextureShaderProgram, "paramWaterLevelThreshold");

    //
    // Create stressed spring program
    //

    mElementStressedSpringShaderProgram = glCreateProgram();

    char const * elementStressedSpringVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        
        // Outputs        
        varying vec2 vertexTextureCoords;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {
            vertexTextureCoords = inputPos; 
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(elementStressedSpringVertexShaderSource, GL_VERTEX_SHADER, mElementStressedSpringShaderProgram);

    char const * elementStressedSpringFragmentShaderSource = R"(
    
        // Inputs
        varying vec2 vertexTextureCoords;

        // Input texture
        uniform sampler2D inputTexture;

        // Params
        uniform float paramAmbientLightIntensity;

        void main()
        {
            gl_FragColor = texture2D(inputTexture, vertexTextureCoords);
        } 
    )";

    GameOpenGL::CompileShader(elementStressedSpringFragmentShaderSource, GL_FRAGMENT_SHADER, mElementStressedSpringShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mElementStressedSpringShaderProgram, 0, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mElementStressedSpringShaderProgram, "Stressed Spring");

    // Get uniform locations
    mElementStressedSpringShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mElementStressedSpringShaderProgram, "paramOrthoMatrix");


    //
    // Create and upload ship texture, if present
    //

    if (!!texture)
    {
        glGenTextures(1, &tmpGLuint);
        mElementTexture = tmpGLuint;

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementTexture);
        glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error binding ship texture: " + std::to_string(glError));
        }

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(*texture));

        //
        // Configure texture
        //

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting wrapping of S coordinate of ship texture: " + std::to_string(glError));
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting wrapping of T coordinate of ship texture: " + std::to_string(glError));
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting minification filter of ship texture: " + std::to_string(glError));
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting magnification filter of ship texture: " + std::to_string(glError));
        }

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }


    //
    // Create stressed spring texture
    //

    // Create texture name
    glGenTextures(1, &tmpGLuint);
    mElementStressedSpringTexture = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mElementStressedSpringTexture);

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Make texture data
    unsigned char buf[] = {
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255,
        255, 253, 181, 255,     239, 16, 39, 255,       255, 253, 181,  255,
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255
    };

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    if (GL_NO_ERROR != glGetError())
    {
        throw GameException("Error uploading stressed spring texture onto GPU");
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Create pinned points program
    //

    // Create program

    mElementPinnedPointShaderProgram = glCreateProgram();

    char const * elementPinnedPointVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        attribute vec2 inputTextureCoords;
        
        // Outputs
        varying vec2 vertexTextureCoords;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {
            vertexTextureCoords = inputTextureCoords; 
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(elementPinnedPointVertexShaderSource, GL_VERTEX_SHADER, mElementPinnedPointShaderProgram);

    char const * elementPinnedPointFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 vertexTextureCoords;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;

        void main()
        {
            vec4 textureColor = texture2D(inputTexture, vertexTextureCoords);
            gl_FragColor = vec4(
                textureColor.xyz * paramAmbientLightIntensity,
                textureColor.w);
        } 
    )";

    GameOpenGL::CompileShader(elementPinnedPointFragmentShaderSource, GL_FRAGMENT_SHADER, mElementPinnedPointShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mElementPinnedPointShaderProgram, PinnedPointPosVertexAttribute, "inputPos");
    glBindAttribLocation(*mElementPinnedPointShaderProgram, PinnedPointTextureCoordinatesVertexAttribute, "inputTextureCoords");

    // Link
    GameOpenGL::LinkShaderProgram(mElementPinnedPointShaderProgram, "Pinned Point");

    // Get uniform locations
    mElementPinnedPointShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mElementPinnedPointShaderProgram, "paramOrthoMatrix");
    mElementPinnedPointShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mElementPinnedPointShaderProgram, "paramAmbientLightIntensity");

    // Create VBO
    GLuint pinnedPointVBO;
    glGenBuffers(1, &pinnedPointVBO);
    mPinnedPointVBO = pinnedPointVBO;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mPinnedPointVBO);

    // Describe buffer
    glVertexAttribPointer(PinnedPointPosVertexAttribute, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(PinnedPointPosVertexAttribute);
    glVertexAttribPointer(PinnedPointTextureCoordinatesVertexAttribute, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(PinnedPointTextureCoordinatesVertexAttribute);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0u);


    //
    // Create bombs program
    //

    // Create program

    mElementBombShaderProgram = glCreateProgram();

    char const * elementBombVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        attribute vec2 inputTextureCoords;
        
        // Outputs
        varying vec2 vertexTextureCoords;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {
            vertexTextureCoords = inputTextureCoords; 
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(elementBombVertexShaderSource, GL_VERTEX_SHADER, mElementBombShaderProgram);

    char const * elementBombFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 vertexTextureCoords;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;

        void main()
        {
            vec4 textureColor = texture2D(inputTexture, vertexTextureCoords);
            gl_FragColor = vec4(
                textureColor.xyz * paramAmbientLightIntensity,
                textureColor.w);
        } 
    )";

    GameOpenGL::CompileShader(elementBombFragmentShaderSource, GL_FRAGMENT_SHADER, mElementBombShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mElementBombShaderProgram, BombPosVertexAttribute, "inputPos");
    glBindAttribLocation(*mElementBombShaderProgram, BombTextureCoordinatesVertexAttribute, "inputTextureCoords");

    // Link
    GameOpenGL::LinkShaderProgram(mElementBombShaderProgram, "Bomb");

    // Get uniform locations
    mElementBombShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mElementBombShaderProgram, "paramOrthoMatrix");
    mElementBombShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mElementBombShaderProgram, "paramAmbientLightIntensity");

    // Create VBO
    GLuint bombVBO;
    glGenBuffers(1, &bombVBO);
    mBombVBO = bombVBO;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mBombVBO);

    // Describe buffer
    glVertexAttribPointer(BombPosVertexAttribute, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(BombPosVertexAttribute);
    glVertexAttribPointer(BombTextureCoordinatesVertexAttribute, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(BombTextureCoordinatesVertexAttribute);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0u);


    //
    // Create vectors program
    //

    // Create program

    mVectorArrowShaderProgram = glCreateProgram();

    char const * vectorArrowVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        
        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);            
        }
    )";

    GameOpenGL::CompileShader(vectorArrowVertexShaderSource, GL_VERTEX_SHADER, mVectorArrowShaderProgram);

    char const * vectorArrowFragmentShaderSource = R"(

        // Parameters        
        uniform vec3 paramColor;

        void main()
        {
            gl_FragColor = vec4(paramColor.xyz, 1.0);
        } 
    )";

    GameOpenGL::CompileShader(vectorArrowFragmentShaderSource, GL_FRAGMENT_SHADER, mVectorArrowShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mVectorArrowShaderProgram, VectorArrowPosVertexAttribute, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mVectorArrowShaderProgram, "Vector Arrow");

    // Get uniform locations
    mVectorArrowShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mVectorArrowShaderProgram, "paramOrthoMatrix");
    mVectorArrowShaderColorParameter = GameOpenGL::GetParameterLocation(mVectorArrowShaderProgram, "paramColor");

    // Create VBO
    GLuint vectorArrowPointPositionVBO;
    glGenBuffers(1, &vectorArrowPointPositionVBO);
    mVectorArrowPointPositionVBO = vectorArrowPointPositionVBO;


    //
    // Set parameters to initial values
    //

    UpdateOrthoMatrix(orthoMatrix);

    UpdateVisibleWorldCoordinates(
        visibleWorldHeight,
        visibleWorldWidth,
        canvasToVisibleWorldHeightRatio);

    UpdateAmbientLightIntensity(ambientLightIntensity);
    UpdateWaterLevelThreshold(waterLevelOfDetail);
}

ShipRenderContext::~ShipRenderContext()
{
}

void ShipRenderContext::UpdateOrthoMatrix(float const(&orthoMatrix)[4][4])
{
    //
    // Set parameter in all programs
    //

    glUseProgram(*mElementColorShaderProgram);
    glUniformMatrix4fv(mElementColorShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(*mElementTextureShaderProgram);
    glUniformMatrix4fv(mElementTextureShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(*mElementColorShaderProgram);
    glUniformMatrix4fv(mElementColorShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(*mElementRopeShaderProgram);
    glUniformMatrix4fv(mElementRopeShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(*mElementStressedSpringShaderProgram);
    glUniformMatrix4fv(mElementStressedSpringShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(*mElementBombShaderProgram);
    glUniformMatrix4fv(mElementBombShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(*mElementPinnedPointShaderProgram);
    glUniformMatrix4fv(mElementPinnedPointShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(*mVectorArrowShaderProgram);
    glUniformMatrix4fv(mVectorArrowShaderOrthoMatrixParameter, 1, GL_FALSE, &(orthoMatrix[0][0]));

    glUseProgram(0);
}

void ShipRenderContext::UpdateVisibleWorldCoordinates(
    float /*visibleWorldHeight*/,
    float /*visibleWorldWidth*/,
    float canvasToVisibleWorldHeightRatio)
{
    mCanvasToVisibleWorldHeightRatio = canvasToVisibleWorldHeightRatio;
}

void ShipRenderContext::UpdateAmbientLightIntensity(float ambientLightIntensity)
{
    mAmbientLightIntensity = ambientLightIntensity;

    //
    // Set parameter in all programs
    //

    glUseProgram(*mElementColorShaderProgram);
    glUniform1f(mElementColorShaderAmbientLightIntensityParameter, ambientLightIntensity);

    glUseProgram(*mElementTextureShaderProgram);
    glUniform1f(mElementTextureShaderAmbientLightIntensityParameter, ambientLightIntensity);

    glUseProgram(*mElementColorShaderProgram);
    glUniform1f(mElementColorShaderAmbientLightIntensityParameter, ambientLightIntensity);

    glUseProgram(*mElementRopeShaderProgram);
    glUniform1f(mElementRopeShaderAmbientLightIntensityParameter, ambientLightIntensity);

    glUseProgram(*mElementPinnedPointShaderProgram);
    glUniform1f(mElementPinnedPointShaderAmbientLightIntensityParameter, ambientLightIntensity);

    glUseProgram(0);
}

void ShipRenderContext::UpdateWaterLevelThreshold(float waterLevelOfDetail)
{
    // Transform: 0->1 == 2.0->0.01
    mWaterLevelThreshold = 2.0f + waterLevelOfDetail * (-2.0f + 0.01f);

    //
    // Set parameter in all programs
    //

    glUseProgram(*mElementColorShaderProgram);
    glUniform1f(mElementColorShaderWaterLevelThresholdParameter, mWaterLevelThreshold);

    glUseProgram(*mElementRopeShaderProgram);
    glUniform1f(mElementRopeShaderWaterLevelThresholdParameter, mWaterLevelThreshold);

    glUseProgram(*mElementTextureShaderProgram);
    glUniform1f(mElementTextureShaderWaterLevelThresholdParameter, mWaterLevelThreshold);

    glUseProgram(0);
}

//////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::UploadPointImmutableGraphicalAttributes(
    size_t count,
    vec3f const * restrict color,
    vec2f const * restrict textureCoordinates)
{
    // Upload colors
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(vec3f), color, GL_STATIC_DRAW);
    glVertexAttribPointer(PointColorVertexAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), (void*)(0));
    glEnableVertexAttribArray(PointColorVertexAttribute);

    if (!!mElementTexture)
    {
        // Upload texture coordinates
        glBindBuffer(GL_ARRAY_BUFFER, *mPointElementTextureCoordinatesVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(vec2f), textureCoordinates, GL_STATIC_DRAW);
        glVertexAttribPointer(PointTextureCoordinatesVertexAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
        glEnableVertexAttribArray(PointTextureCoordinatesVertexAttribute);
    }    

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0u);

    // Store size (for later assert)
    mPointCount = count;
}

void ShipRenderContext::UploadPoints(
    size_t count,
    vec2f const * restrict position,
    float const * restrict light,
    float const * restrict water)
{
    assert(count == mPointCount);

    // Upload positions
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPositionVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(vec2f), position, GL_STREAM_DRAW);
    glVertexAttribPointer(PointPosVertexAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    glEnableVertexAttribArray(PointPosVertexAttribute);

    // Upload lights
    glBindBuffer(GL_ARRAY_BUFFER, *mPointLightVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), light, GL_STREAM_DRAW);
    glVertexAttribPointer(PointLightVertexAttribute, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
    glEnableVertexAttribArray(PointLightVertexAttribute);

    // Upload waters
    glBindBuffer(GL_ARRAY_BUFFER, *mPointWaterVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), water, GL_STREAM_DRAW);
    glVertexAttribPointer(PointWaterVertexAttribute, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
    glEnableVertexAttribArray(PointWaterVertexAttribute);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0u);
}

void ShipRenderContext::UploadElementsStart(std::vector<std::size_t> const & connectedComponentsMaxSizes)
{
    GLuint elementVBO;

    if (connectedComponentsMaxSizes.size() != mConnectedComponents.size())
    {
        // A change in the number of connected components, nuke everything
        mConnectedComponents.clear();
        mConnectedComponents.resize(connectedComponentsMaxSizes.size());
    }
    
    for (size_t c = 0; c < mConnectedComponents.size(); ++c)
    {
        //
        // Prepare point elements
        //

        // Max # of points = number of points
        size_t maxConnectedComponentPoints = connectedComponentsMaxSizes[c];
        if (mConnectedComponents[c].pointElementMaxCount != maxConnectedComponentPoints)
        {
            // A change in the max size of this connected component
            mConnectedComponents[c].pointElementBuffer.reset();
            mConnectedComponents[c].pointElementBuffer.reset(new PointElement[maxConnectedComponentPoints]);
            mConnectedComponents[c].pointElementMaxCount = maxConnectedComponentPoints;
        }

        mConnectedComponents[c].pointElementCount = 0;

        if (!mConnectedComponents[c].pointElementVBO)
        {
            glGenBuffers(1, &elementVBO);
            mConnectedComponents[c].pointElementVBO = elementVBO;
        }
        
        //
        // Prepare spring elements
        //

        // Max # of springs = number of points * 9 (8 neighbours plus one rope for endpoint points)
        size_t maxConnectedComponentSprings = connectedComponentsMaxSizes[c] * 9;        
        if (mConnectedComponents[c].springElementMaxCount != maxConnectedComponentSprings)
        {
            // A change in the max size of this connected component
            mConnectedComponents[c].springElementBuffer.reset();
            mConnectedComponents[c].springElementBuffer.reset(new SpringElement[maxConnectedComponentSprings]);
            mConnectedComponents[c].springElementMaxCount = maxConnectedComponentSprings;
        }

        mConnectedComponents[c].springElementCount = 0;

        if (!mConnectedComponents[c].springElementVBO)
        {
            glGenBuffers(1, &elementVBO);
            mConnectedComponents[c].springElementVBO = elementVBO;
        }

        //
        // Prepare rope elements
        //

        // Max # of ropes = max number of springs
        size_t maxConnectedComponentRopes = maxConnectedComponentSprings;
        if (mConnectedComponents[c].ropeElementMaxCount != maxConnectedComponentRopes)
        {
            // A change in the max size of this connected component
            mConnectedComponents[c].ropeElementBuffer.reset();
            mConnectedComponents[c].ropeElementBuffer.reset(new RopeElement[maxConnectedComponentRopes]);
            mConnectedComponents[c].ropeElementMaxCount = maxConnectedComponentRopes;
        }

        mConnectedComponents[c].ropeElementCount = 0;

        if (!mConnectedComponents[c].ropeElementVBO)
        {
            glGenBuffers(1, &elementVBO);
            mConnectedComponents[c].ropeElementVBO = elementVBO;
        }

        //
        // Prepare triangle elements
        //

        // Max # of triangles = number of points * 8 (each of the 8 directions)
        size_t maxConnectedComponentTriangles = connectedComponentsMaxSizes[c] * 8;
        if (mConnectedComponents[c].triangleElementMaxCount != maxConnectedComponentTriangles)
        {
            // A change in the max size of this connected component
            mConnectedComponents[c].triangleElementBuffer.reset();
            mConnectedComponents[c].triangleElementBuffer.reset(new TriangleElement[maxConnectedComponentTriangles]);
            mConnectedComponents[c].triangleElementMaxCount = maxConnectedComponentTriangles;
        }

        mConnectedComponents[c].triangleElementCount = 0;

        if (!mConnectedComponents[c].triangleElementVBO)
        {
            glGenBuffers(1, &elementVBO);
            mConnectedComponents[c].triangleElementVBO = elementVBO;
        }

        //
        // Prepare stressed spring elements
        //

        // Max # of stressed springs = max number of springs
        size_t maxConnectedComponentStressedSprings = maxConnectedComponentSprings;
        if (mConnectedComponents[c].stressedSpringElementMaxCount != maxConnectedComponentStressedSprings)
        {
            // A change in the max size of this connected component
            mConnectedComponents[c].stressedSpringElementBuffer.reset();
            mConnectedComponents[c].stressedSpringElementBuffer.reset(new StressedSpringElement[maxConnectedComponentStressedSprings]);
            mConnectedComponents[c].stressedSpringElementMaxCount = maxConnectedComponentStressedSprings;
        }

        mConnectedComponents[c].stressedSpringElementCount = 0;

        if (!mConnectedComponents[c].stressedSpringElementVBO)
        {
            glGenBuffers(1, &elementVBO);
            mConnectedComponents[c].stressedSpringElementVBO = elementVBO;
        }
        
        //
        // Prepare pinned point elements
        //

        mConnectedComponents[c].pinnedPointElementOffset = 0;
        mConnectedComponents[c].pinnedPointElementCount = 0;

        //
        // Prepare bomb elements
        //

        mConnectedComponents[c].bombElementOffset = 0;
        mConnectedComponents[c].bombElementInfos.clear();
    }
}

void ShipRenderContext::UploadElementsEnd()
{
    //
    // Upload all elements, except for stressed springs, pinned points, and bombs
    //

    for (size_t c = 0; c < mConnectedComponents.size(); ++c)
    {
        // Points
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].pointElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].pointElementCount * sizeof(PointElement), mConnectedComponents[c].pointElementBuffer.get(), GL_STATIC_DRAW);

        // Springs
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].springElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].springElementCount * sizeof(SpringElement), mConnectedComponents[c].springElementBuffer.get(), GL_STATIC_DRAW);

        // Ropes
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].ropeElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].ropeElementCount * sizeof(RopeElement), mConnectedComponents[c].ropeElementBuffer.get(), GL_STATIC_DRAW);

        // Triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].triangleElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].triangleElementCount * sizeof(TriangleElement), mConnectedComponents[c].triangleElementBuffer.get(), GL_STATIC_DRAW);
    }
}

void ShipRenderContext::UploadElementStressedSpringsStart()
{
    for (size_t c = 0; c < mConnectedComponents.size(); ++c)
    {
        // Zero-out count of stressed springs
        mConnectedComponents[c].stressedSpringElementCount = 0;
    }
}

void ShipRenderContext::UploadElementStressedSpringsEnd()
{
    //
    // Upload stressed spring elements
    //

    for (size_t c = 0; c < mConnectedComponents.size(); ++c)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].stressedSpringElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].stressedSpringElementCount * sizeof(StressedSpringElement), mConnectedComponents[c].stressedSpringElementBuffer.get(), GL_DYNAMIC_DRAW);
    }
}

void ShipRenderContext::UploadElementPinnedPointsStart(size_t count)
{
    // Clear per-connected component metadata
    for (auto c = 0; c < mConnectedComponents.size(); ++c)
    {
        mConnectedComponents[c].pinnedPointElementOffset = 0;
        mConnectedComponents[c].pinnedPointElementCount = 0;
    }

    // Clear pinned point buffer
    mPinnedPointElementBuffer.clear();
    mPinnedPointElementBuffer.reserve(count);
}

void ShipRenderContext::UploadElementPinnedPointsEnd()
{
    //
    // Upload pinned points
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mPinnedPointVBO);
    glBufferData(GL_ARRAY_BUFFER, mPinnedPointElementBuffer.size() * sizeof(PinnedPointElement), mPinnedPointElementBuffer.data(), GL_STATIC_DRAW);
}

void ShipRenderContext::UploadElementBombsStart(size_t count)
{
    // Clear per-connected component metadata
    for (auto c = 0; c < mConnectedComponents.size(); ++c)
    {
        mConnectedComponents[c].bombElementOffset = 0;
        mConnectedComponents[c].bombElementInfos.clear();
    }

    // Clear bomb buffer
    mBombElementBuffer.clear();
    mBombElementBuffer.reserve(count);
}

void ShipRenderContext::UploadElementBombsEnd()
{
    //
    // Upload bombs
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mBombVBO);
    glBufferData(GL_ARRAY_BUFFER, mBombElementBuffer.size() * sizeof(BombElement), mBombElementBuffer.data(), GL_STATIC_DRAW);
}

void ShipRenderContext::UploadVectors(
    size_t count,
    vec2f const * restrict position,
    vec2f const * restrict vector,
    float lengthAdjustment,
    vec3f const & color)
{
    static float const CosAlphaLeftRight = cos(-2.f * Pi<float> / 8.f);
    static float const SinAlphaLeft = sin(-2.f * Pi<float> / 8.f);
    static float const SinAlphaRight = -SinAlphaLeft;

    static vec2f const XMatrixLeft = vec2f(CosAlphaLeftRight, SinAlphaLeft);
    static vec2f const YMatrixLeft = vec2f(-SinAlphaLeft, CosAlphaLeftRight);
    static vec2f const XMatrixRight = vec2f(CosAlphaLeftRight, SinAlphaRight);
    static vec2f const YMatrixRight = vec2f(-SinAlphaRight, CosAlphaLeftRight);

    //
    // Create buffer with endpoint positions of each segment of each arrow
    //

    mVectorArrowPointPositionBuffer.clear();
    mVectorArrowPointPositionBuffer.reserve(count * 3 * 2);

    for (size_t i = 0; i < count; ++i)
    {
        // Stem
        vec2f stemEndpoint = position[i] + vector[i] * lengthAdjustment;
        mVectorArrowPointPositionBuffer.push_back(position[i]);
        mVectorArrowPointPositionBuffer.push_back(stemEndpoint);

        // Left
        vec2f leftDir = vec2f(-vector[i].dot(XMatrixLeft), -vector[i].dot(YMatrixLeft)).normalise();
        mVectorArrowPointPositionBuffer.push_back(stemEndpoint);
        mVectorArrowPointPositionBuffer.push_back(stemEndpoint + leftDir * 0.2f);

        // Right
        vec2f rightDir = vec2f(-vector[i].dot(XMatrixRight), -vector[i].dot(YMatrixRight)).normalise();
        mVectorArrowPointPositionBuffer.push_back(stemEndpoint);
        mVectorArrowPointPositionBuffer.push_back(stemEndpoint + rightDir * 0.2f);
    }


    //
    // Upload buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowPointPositionVBO);
    glBufferData(GL_ARRAY_BUFFER, mVectorArrowPointPositionBuffer.size() * sizeof(vec2f), mVectorArrowPointPositionBuffer.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(VectorArrowPosVertexAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    glEnableVertexAttribArray(VectorArrowPosVertexAttribute);
    glBindBuffer(GL_ARRAY_BUFFER, 0u);


    //
    // Set color parameter
    //

    glUseProgram(*mVectorArrowShaderProgram);
    glUniform3f(mVectorArrowShaderColorParameter, color.x, color.y, color.z);
    glUseProgram(0);
}

void ShipRenderContext::UploadLampsStart(size_t connectedComponents)
{
    mLampBuffers.clear();
    mLampBuffers.resize(connectedComponents);
}

void ShipRenderContext::UploadLampsEnd()
{
    // Nop
}

void ShipRenderContext::Render(
    ShipRenderMode shipRenderMode,
    VectorFieldRenderMode vectorFieldRenderMode,
    bool showStressedSprings)
{
    //
    // Process all connected components, from first to last, and draw all elements
    //

    for (size_t c = 0; c < mConnectedComponents.size(); ++c)
    {
        //
        // Draw points
        //

        if (shipRenderMode == ShipRenderMode::Points)
        {
            RenderPointElements(mConnectedComponents[c]);
        }


        //
        // Draw springs
        //
        // We draw springs when:
        // - RenderMode is springs ("X-Ray Mode"), in which case we use colors - so to show structural springs -, or
        // - RenderMode is structure (so to draw 1D chains), in which case we use colors, or
        // - RenderMode is texture (so to draw 1D chains), in which case we use texture iff it is present
        //

        if (shipRenderMode == ShipRenderMode::Springs
            || shipRenderMode == ShipRenderMode::Structure
            || shipRenderMode == ShipRenderMode::Texture)
        {
            RenderSpringElements(
                mConnectedComponents[c],
                shipRenderMode == ShipRenderMode::Texture);
        }


        //
        // Draw ropes now if RenderMode is:
        // - Springs
        // - Texture (so rope endpoints are hidden behind texture, looks better)
        //

        if (shipRenderMode == ShipRenderMode::Springs
            || shipRenderMode == ShipRenderMode::Texture)
        {
            RenderRopeElements(mConnectedComponents[c]);
        }


        //
        // Draw triangles
        //

        if (shipRenderMode == ShipRenderMode::Structure
            || shipRenderMode == ShipRenderMode::Texture)
        {
            RenderTriangleElements(
                mConnectedComponents[c],
                shipRenderMode == ShipRenderMode::Texture);
        }


        //
        // Draw ropes now if RenderMode is Structure (so rope endpoints on the structure are visible)
        //

        if (shipRenderMode == ShipRenderMode::Structure)
        {
            RenderRopeElements(mConnectedComponents[c]);
        }


        //
        // Draw stressed springs
        //

        if (showStressedSprings)
        {
            RenderStressedSpringElements(mConnectedComponents[c]);
        }


        //
        // Draw bombs
        //

        RenderBombElements(mConnectedComponents[c]);


        //
        // Draw pinned points
        //

        RenderPinnedPointElements(mConnectedComponents[c]);
    }


    //
    // Render vectors, if we're asked to
    //

    if (vectorFieldRenderMode != VectorFieldRenderMode::None)
    {
        RenderVectors();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::RenderPointElements(ConnectedComponentData const & connectedComponent)
{
    // Use color program
    glUseProgram(*mElementColorShaderProgram);

    // Set point size
    glPointSize(0.2f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.pointElementVBO);

    // Draw
    glDrawElements(GL_POINTS, static_cast<GLsizei>(1 * connectedComponent.pointElementCount), GL_UNSIGNED_INT, 0);

    // Stop using program
    glUseProgram(0);
}

void ShipRenderContext::RenderSpringElements(
    ConnectedComponentData const & connectedComponent,
    bool withTexture)
{
    if (withTexture && !!mElementTexture)
    {
        // Use texture program
        glUseProgram(*mElementTextureShaderProgram);
        
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementTexture);
    }
    else
    {
        // Use color program
        glUseProgram(*mElementColorShaderProgram);
    }

    // Set line size
    glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.springElementVBO);

    // Draw
    glDrawElements(GL_LINES, static_cast<GLsizei>(2 * connectedComponent.springElementCount), GL_UNSIGNED_INT, 0);

    // Unbind texture (if any)
    if (withTexture && !!mElementTexture)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Stop using program
    glUseProgram(0);
}

void ShipRenderContext::RenderRopeElements(ConnectedComponentData const & connectedComponent)
{
    // Use rope program
    glUseProgram(*mElementRopeShaderProgram);

    // Set line size
    glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.ropeElementVBO);

    // Draw
    glDrawElements(GL_LINES, static_cast<GLsizei>(2 * connectedComponent.ropeElementCount), GL_UNSIGNED_INT, 0);

    // Stop using program
    glUseProgram(0);
}

void ShipRenderContext::RenderTriangleElements(
    ConnectedComponentData const & connectedComponent,
    bool withTexture)
{
    if (withTexture && !!mElementTexture)
    {
        // Use texture program
        glUseProgram(*mElementTextureShaderProgram);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementTexture);
    }
    else
    {
        // Use color program
        glUseProgram(*mElementColorShaderProgram);
    }

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.triangleElementVBO);

    // Draw
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(3 * connectedComponent.triangleElementCount), GL_UNSIGNED_INT, 0);

    // Unbind texture (if any)
    if (withTexture && !!mElementTexture)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Stop using program
    glUseProgram(0);
}

void ShipRenderContext::RenderStressedSpringElements(ConnectedComponentData const & connectedComponent)
{
    // Use program
    glUseProgram(*mElementStressedSpringShaderProgram);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mElementStressedSpringTexture);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.stressedSpringElementVBO);

    // Set line size
    glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Draw
    glDrawElements(GL_LINES, static_cast<GLsizei>(2 * connectedComponent.stressedSpringElementCount), GL_UNSIGNED_INT, 0);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Stop using program
    glUseProgram(0);
}

void ShipRenderContext::RenderBombElements(ConnectedComponentData const & connectedComponent)
{
    // Use program
    glUseProgram(*mElementBombShaderProgram);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mBombVBO);

    // Draw all bombs for this connected component
    for (size_t b = 0; b < connectedComponent.bombElementInfos.size(); ++b)
    {
        if (!!connectedComponent.bombElementInfos[b].lightedFrameId)
        {
            //
            // Lighted element
            //

            // Set light parameter
            glUniform1f(mElementBombShaderAmbientLightIntensityParameter, mAmbientLightIntensity);

            // Bind texture
            glBindTexture(
                GL_TEXTURE_2D,
                mTextureRenderManager.GetOpenGLHandle(*connectedComponent.bombElementInfos[b].lightedFrameId));

            // Draw
            glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(4 * (connectedComponent.bombElementOffset + b)), 4);
        }

        if (!!connectedComponent.bombElementInfos[b].unlightedFrameId)
        {
            //
            // Unlighted element
            //

            // Set light parameter
            glUniform1f(mElementBombShaderAmbientLightIntensityParameter, 1.0f);

            // Bind texture
            glBindTexture(
                GL_TEXTURE_2D,
                mTextureRenderManager.GetOpenGLHandle(*connectedComponent.bombElementInfos[b].unlightedFrameId));

            // Draw
            glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(4 * (connectedComponent.bombElementOffset + b)), 4);
        }

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Stop using program
    glUseProgram(0);
}

void ShipRenderContext::RenderPinnedPointElements(ConnectedComponentData const & connectedComponent)
{
    // Use program
    glUseProgram(*mElementPinnedPointShaderProgram);

    // Bind texture
    glBindTexture(
        GL_TEXTURE_2D, 
        mTextureRenderManager.GetOpenGLHandle(TextureGroupType::PinnedPoint, 0));

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mPinnedPointVBO);

    // Draw
    for (size_t p = 0; p < connectedComponent.pinnedPointElementCount; ++p)
    {
        glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(4 * (connectedComponent.pinnedPointElementOffset + p)), 4);
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Stop using program
    glUseProgram(0);
}

void ShipRenderContext::RenderVectors()
{
    // Use vector arrow program
    glUseProgram(*mVectorArrowShaderProgram);
    
    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowPointPositionVBO);

    // Set line size
    glLineWidth(0.5f);

    // Draw
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mVectorArrowPointPositionBuffer.size()));
    
    // Stop using program
    glUseProgram(0);
}