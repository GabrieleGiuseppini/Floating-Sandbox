/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include "GameException.h"

#include <cstring>

RenderContext::RenderContext(
    ResourceLoader & resourceLoader,
    vec3f const & ropeColour,
    ProgressCallback const & progressCallback)
    : // Clouds
      mCloudShaderProgram()
    , mCloudShaderAmbientLightIntensityParameter(0)
    , mCloudBuffer()
    , mCloudBufferSize(0u)
    , mCloudBufferMaxSize(0u)    
    , mCloudVBO()
    , mCloudTextureSizes()
    , mCloudTextures()
    // Land
    , mLandShaderProgram()
    , mLandShaderAmbientLightIntensityParameter(0)
    , mLandShaderOrthoMatrixParameter(0)
    , mLandBuffer()
    , mLandBufferSize(0u)
    , mLandBufferMaxSize(0u)
    , mLandVBO()
    , mLandTexture()
    // Sea water
    , mWaterShaderProgram()
    , mWaterShaderAmbientLightIntensityParameter(0)
    , mWaterShaderWaterTransparencyParameter(0)
    , mWaterShaderOrthoMatrixParameter(0)
    , mWaterBuffer()
    , mWaterBufferSize(0u)
    , mWaterBufferMaxSize(0u)
    , mWaterVBO()
    , mWaterTexture()
    // Ships
    , mShips()
    , mRopeColour(ropeColour)
    , mPinnedPointTextureSize(0, 0)
    , mPinnedPointTexture()
    , mRCBombTextureSizes()
    , mRCBombTextures()
    , mTimerBombTextureSizes()
    , mTimerBombTextures()
    // Multi-purpose shaders
    , mMatteNdcShaderProgram()
    , mMatteNdcShaderColorParameter(0)
    , mMatteNdcVBO()
    , mMatteWorldShaderProgram()
    , mMatteWorldShaderColorParameter(0)
    , mMatteWorldShaderOrthoMatrixParameter(0)
    , mMatteWorldVBO()
    // Render parameters
    , mZoom(1.0f)
    , mCamX(0.0f)
    , mCamY(0.0f)
    , mCanvasWidth(100)
    , mCanvasHeight(100)
    , mAmbientLightIntensity(1.0f)
    , mSeaWaterTransparency(0.8125f)
    , mShowShipThroughSeaWater(false)
    , mWaterLevelOfDetail(0.6875f)
    , mShipRenderMode(ShipRenderMode::Texture)
    , mShowStressedSprings(false)
{
    GLuint tmpGLuint;

    //
    // Init OpenGL
    //

    GameOpenGL::InitOpenGL();

    // Activate texture unit 0
    glActiveTexture(GL_TEXTURE0);


    //
    // Load textures
    //

    auto cloudTextureDatas = resourceLoader.LoadTexturesRgba(
        "cloud",
        [&progressCallback](float progress, std::string const &)
        {
            if (progressCallback)
                progressCallback(progress / 6.0f, "Loading textures...");
        });

    if (progressCallback)
        progressCallback(2.0f/6.0f, "Loading textures...");

    auto landTextureData = resourceLoader.LoadTextureRgba(std::string("sand_1.jpg"));

    if (progressCallback)
        progressCallback(3.0f / 6.0f, "Loading textures...");

    auto waterTextureData = resourceLoader.LoadTextureRgba(std::string("water_1.jpg"));

    if (progressCallback)
        progressCallback(4.0f / 6.0f, "Loading textures...");

    auto pinnedPointTextureData = resourceLoader.LoadTextureRgba(std::string("pin_down_1.png"));

    auto rcBombTextureDatas = resourceLoader.LoadTexturesRgba(
        "rc_bomb",
        [&progressCallback](float progress, std::string const &)
        {
            if (progressCallback)
                progressCallback((4.0f + progress) / 6.0f, "Loading textures...");
        });

    auto timerBombTextureDatas = resourceLoader.LoadTexturesRgba(
        "timer_bomb",
        [&progressCallback](float progress, std::string const &)
    {
        if (progressCallback)
            progressCallback((5.0f + progress) / 6.0f, "Loading textures...");
    });


    //
    // Clouds 
    //

    // Create program

    mCloudShaderProgram = glCreateProgram();

    char const * cloudVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        attribute vec2 inputTexturePos;
        
        // Outputs
        varying vec2 texturePos;

        void main()
        {
            gl_Position = vec4(inputPos.xy, -1.0, 1.0);
            texturePos = inputTexturePos;
        }
    )";

    GameOpenGL::CompileShader(cloudVertexShaderSource, GL_VERTEX_SHADER, mCloudShaderProgram);

    char const * cloudFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 texturePos;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;

        void main()
        {
            gl_FragColor = texture2D(inputTexture, texturePos) * paramAmbientLightIntensity;
        } 
    )";

    GameOpenGL::CompileShader(cloudFragmentShaderSource, GL_FRAGMENT_SHADER, mCloudShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mCloudShaderProgram, 0, "inputPos");
    glBindAttribLocation(*mCloudShaderProgram, 1, "inputTexturePos");

    // Link
    GameOpenGL::LinkShaderProgram(mCloudShaderProgram, "Cloud");

    // Get uniform locations
    mCloudShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mCloudShaderProgram, "paramAmbientLightIntensity");

    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mCloudVBO = tmpGLuint;

    // Set hardcoded parameters
    glUseProgram(*mCloudShaderProgram);
    glUseProgram(0);

    // Create textures
    for (size_t i = 0; i < cloudTextureDatas.size(); ++i)
    {
        // Create texture name
        glGenTextures(1, &tmpGLuint);
        mCloudTextures.emplace_back(tmpGLuint);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mCloudTextures.back());

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cloudTextureDatas[i].Size.Width, cloudTextureDatas[i].Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cloudTextureDatas[i].Data.get());
        if (GL_NO_ERROR != glGetError())
        {
            throw GameException("Error uploading cloud texture onto GPU");
        }

        // Store size
        mCloudTextureSizes.push_back(cloudTextureDatas[i].Size);

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    assert(mCloudTextureSizes.size() == mCloudTextures.size());

    //
    // Land 
    //

    // Create program

    mLandShaderProgram = glCreateProgram();

    char const * landVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        
        // Parameters
        uniform mat4 paramOrthoMatrix;

        // Outputs
        varying vec2 texturePos;

        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
            texturePos = inputPos.xy;
        }
    )";

    GameOpenGL::CompileShader(landVertexShaderSource, GL_VERTEX_SHADER, mLandShaderProgram);

    char const * landFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 texturePos;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;
        uniform vec2 paramTextureScaling;

        void main()
        {
            gl_FragColor = texture2D(inputTexture, texturePos * paramTextureScaling) * paramAmbientLightIntensity;
        } 
    )";

    GameOpenGL::CompileShader(landFragmentShaderSource, GL_FRAGMENT_SHADER, mLandShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mLandShaderProgram, 0, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mLandShaderProgram, "Land");

    // Get uniform locations
    mLandShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mLandShaderProgram, "paramAmbientLightIntensity");
    GLint landShaderTextureScalingParameter = GameOpenGL::GetParameterLocation(mLandShaderProgram, "paramTextureScaling");
    mLandShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mLandShaderProgram, "paramOrthoMatrix");

    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mLandVBO = tmpGLuint;

    // Calculate scaling: we want a tile to be repeated every these many world units
    static constexpr float LandTileWorldSize = 128.0f;
    
    // Set hardcoded parameters
    glUseProgram(*mLandShaderProgram);
    glUniform2f(
        landShaderTextureScalingParameter, 
        1.0f / LandTileWorldSize,
        1.0f / LandTileWorldSize);
    glUseProgram(0);

    // Create texture name
    glGenTextures(1, &tmpGLuint);
    mLandTexture = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mLandTexture);

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, landTextureData.Size.Width, landTextureData.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, landTextureData.Data.get());
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw GameException("Error uploading land texture onto GPU: " + std::to_string(glError));
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Water 
    //

    // Create program

    mWaterShaderProgram = glCreateProgram();

    char const * waterVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        attribute float inputTextureY;

        // Parameters
        uniform mat4 paramOrthoMatrix;

        // Outputs
        varying vec2 texturePos;

        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
            texturePos = vec2(inputPos.x, inputTextureY);
        }
    )";

    GameOpenGL::CompileShader(waterVertexShaderSource, GL_VERTEX_SHADER, mWaterShaderProgram);

    char const * waterFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 texturePos;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;
        uniform float paramWaterTransparency;
        uniform vec2 paramTextureScaling;

        void main()
        {
            vec4 textureColor = texture2D(inputTexture, texturePos * paramTextureScaling);
            gl_FragColor = vec4(textureColor.xyz, 1.0 - paramWaterTransparency) * paramAmbientLightIntensity;
        } 
    )";

    GameOpenGL::CompileShader(waterFragmentShaderSource, GL_FRAGMENT_SHADER, mWaterShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mWaterShaderProgram, 0, "inputPos");
    glBindAttribLocation(*mWaterShaderProgram, 1, "inputTextureY");

    // Link
    GameOpenGL::LinkShaderProgram(mWaterShaderProgram, "Water");

    // Get uniform locations    
    mWaterShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramAmbientLightIntensity");
    mWaterShaderWaterTransparencyParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramWaterTransparency");
    GLint waterShaderTextureScalingParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramTextureScaling");
    mWaterShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramOrthoMatrix");

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mWaterVBO = tmpGLuint;

    // Calculate scaling: we want a tile to be repeated every these many world units
    static constexpr float WaterTileWorldSize = 128.0f;

    // Set hardcoded parameters
    glUseProgram(*mWaterShaderProgram);
    glUniform2f(
        waterShaderTextureScalingParameter,
        1.0f / WaterTileWorldSize,
        1.0f / WaterTileWorldSize);
    glUseProgram(0);

    // Create texture name
    glGenTextures(1, &tmpGLuint);
    mWaterTexture = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mWaterTexture);

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, waterTextureData.Size.Width, waterTextureData.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, waterTextureData.Data.get());
    if (GL_NO_ERROR != glGetError())
    {
        throw GameException("Error uploading water texture onto GPU");
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Pinned points
    //

    // Store texture size
    mPinnedPointTextureSize = pinnedPointTextureData.Size;

    // Create texture name
    glGenTextures(1, &tmpGLuint);
    mPinnedPointTexture = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mPinnedPointTexture);

    // Upload texture
    GameOpenGL::UploadMipmappedTexture(std::move(pinnedPointTextureData));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // RC Bomb
    //

    // Create textures
    for (size_t i = 0; i < rcBombTextureDatas.size(); ++i)
    {
        // Store size
        mRCBombTextureSizes.push_back(rcBombTextureDatas[i].Size);

        // Create texture name
        glGenTextures(1, &tmpGLuint);
        mRCBombTextures.emplace_back(tmpGLuint);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mRCBombTextures.back());

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(rcBombTextureDatas[i]));

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }


    //
    // Timer Bomb
    //

    // Create textures
    for (size_t i = 0; i < timerBombTextureDatas.size(); ++i)
    {
        // Store size
        mTimerBombTextureSizes.push_back(timerBombTextureDatas[i].Size);

        // Create texture name
        glGenTextures(1, &tmpGLuint);
        mTimerBombTextures.emplace_back(tmpGLuint);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mTimerBombTextures.back());

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(timerBombTextureDatas[i]));

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }


    //
    // Multi-purpose Matte NDC shader
    //

    mMatteNdcShaderProgram = glCreateProgram();

    char const * matteNdcShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;

        void main()
        {
            gl_Position = vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(matteNdcShaderSource, GL_VERTEX_SHADER, mMatteNdcShaderProgram);

    char const * matteNdcFragmentShaderSource = R"(

        // Params
        uniform vec4 paramCol;

        void main()
        {
            gl_FragColor = paramCol;
        } 
    )";

    GameOpenGL::CompileShader(matteNdcFragmentShaderSource, GL_FRAGMENT_SHADER, mMatteNdcShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mMatteNdcShaderProgram, 0, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mMatteNdcShaderProgram, "Matte NDC");

    // Get uniform locations
    mMatteNdcShaderColorParameter = GameOpenGL::GetParameterLocation(mMatteNdcShaderProgram, "paramCol");

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mMatteNdcVBO = tmpGLuint;


    //
    // Multi-purpose Matte World shader
    //

    mMatteWorldShaderProgram = glCreateProgram();

    char const * matteWorldShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(matteWorldShaderSource, GL_VERTEX_SHADER, mMatteWorldShaderProgram);

    char const * matteWorldFragmentShaderSource = R"(

        // Params
        uniform vec4 paramCol;

        void main()
        {
            gl_FragColor = paramCol;
        } 
    )";

    GameOpenGL::CompileShader(matteWorldFragmentShaderSource, GL_FRAGMENT_SHADER, mMatteWorldShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mMatteWorldShaderProgram, 0, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mMatteWorldShaderProgram, "Matte World");

    // Get uniform locations
    mMatteWorldShaderColorParameter = GameOpenGL::GetParameterLocation(mMatteWorldShaderProgram, "paramCol");
    mMatteWorldShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mMatteWorldShaderProgram, "paramOrthoMatrix");

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mMatteWorldVBO = tmpGLuint;


    //
    // Initialize ortho matrix
    //

    for (size_t r = 0; r < 4; ++r)
    {
        for (size_t c = 0; c < 4; ++c)
        {
            mOrthoMatrix[r][c] = 0.0f;
        }
    }


    //
    // Update parameters
    //

    UpdateOrthoMatrix();
    UpdateVisibleWorldCoordinates();
    UpdateAmbientLightIntensity();
    UpdateSeaWaterTransparency();
    UpdateWaterLevelOfDetail();


    //
    // Flush all pending operations
    //

    glFlush();
}

RenderContext::~RenderContext()
{
    glUseProgram(0u);
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::Reset()
{
    // Clear ships
    mShips.clear();

    // TODO: might also reset all the other render contextes, once we have refactored
    // them out and stored them as uq_ptr's
}

void RenderContext::AddShip(
    int shipId,
    std::optional<ImageData> texture)
{   
    assert(shipId == mShips.size());

    // Prepare RC bomb texture data
    std::vector<GLuint> rcBombTextures;
    for (size_t i = 0; i < mRCBombTextures.size(); ++i)
    {
        rcBombTextures.push_back(*mRCBombTextures[i]);
    }

    // Prepare Timer bomb texture data
    std::vector<GLuint> timerBombTextures;
    for (size_t i = 0; i < mTimerBombTextures.size(); ++i)
    {
        timerBombTextures.push_back(*mTimerBombTextures[i]);
    }

    // Add the ship    
    mShips.emplace_back(
        new ShipRenderContext(
            std::move(texture), 
            mRopeColour,
            *mPinnedPointTexture,
            rcBombTextures,
            timerBombTextures,
            mOrthoMatrix,
            mVisibleWorldHeight,
            mVisibleWorldWidth,
            mCanvasToVisibleWorldHeightRatio,
            mAmbientLightIntensity,
            mWaterLevelOfDetail));
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderStart()
{
    // Set anti-aliasing for lines and polygons
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Enable blend for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable stencil test    
    glEnable(GL_STENCIL_TEST);    

    // Clear canvas 
    static const vec3f ClearColorBase(0.529f, 0.808f, 0.980f); // (cornflower blue)
    vec3f clearColor = ClearColorBase * mAmbientLightIntensity;
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);
}

void RenderContext::RenderCloudsStart(size_t clouds)
{
    if (clouds != mCloudBufferMaxSize)
    {
        // Realloc
        mCloudBuffer.reset();
        mCloudBuffer.reset(new CloudElement[clouds]);
        mCloudBufferMaxSize = clouds;
    }

    mCloudBufferSize = 0u;
}

void RenderContext::RenderCloudsEnd()
{
    //
    // Draw stencil
    //

    // Use matte world program
    glUseProgram(*mMatteWorldShaderProgram);

    // Set parameters
    glUniform4f(mMatteWorldShaderColorParameter, 1.0f, 1.0f, 1.0f, 1.0f);

    // Bind water buffer
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Disable writing to the color buffer
    glColorMask(false, false, false, false);

    // Write all one's to stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterBufferSize));

    // Don't write anything to stencil buffer now
    glStencilMask(0x00);

    // Re-enable writing to the color buffer
    glColorMask(true, true, true, true);
    
    // Stop using program
    glUseProgram(0);



    //
    // Draw clouds
    //

    assert(mCloudBufferSize == mCloudBufferMaxSize);

    // Use program
    glUseProgram(*mCloudShaderProgram);

    // Upload cloud buffer 
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    glBufferData(GL_ARRAY_BUFFER, mCloudBufferSize * sizeof(CloudElement), mCloudBuffer.get(), GL_DYNAMIC_DRAW);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Describe InputTexturePos
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Enable stenciling - only draw where there are no 1's
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    // Draw
    for (size_t c = 0; c < mCloudBufferSize; ++c)
    {
        // Bind Texture
        glBindTexture(GL_TEXTURE_2D, *(mCloudTextures[GetCloudTextureIndex(c)]));

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(4 * c), 4);

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Disable stenciling - draw always
    glStencilFunc(GL_ALWAYS, 0, 0x00);

    // Stop using program
    glUseProgram(0);
}

void RenderContext::UploadLandAndWaterStart(size_t slices)
{
    //
    // Prepare land buffer
    //

    if (slices + 1 != mLandBufferMaxSize)
    {
        // Realloc
        mLandBuffer.reset();
        mLandBuffer.reset(new LandElement[slices + 1]);
        mLandBufferMaxSize = slices + 1;
    }

    mLandBufferSize = 0u;

    //
    // Prepare water buffer
    //

    if (slices + 1 != mWaterBufferMaxSize)
    {
        // Realloc
        mWaterBuffer.reset();
        mWaterBuffer.reset(new WaterElement[slices + 1]);
        mWaterBufferMaxSize = slices + 1;
    }

    mWaterBufferSize = 0u;
}

void RenderContext::UploadLandAndWaterEnd()
{
    //
    // Upload land buffer
    //

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    // Upload land buffer
    glBufferData(GL_ARRAY_BUFFER, mLandBufferSize * sizeof(LandElement), mLandBuffer.get(), GL_DYNAMIC_DRAW);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0u);


    //
    // Upload water buffer
    //

    assert(mWaterBufferSize == mWaterBufferMaxSize);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Upload water buffer
    glBufferData(GL_ARRAY_BUFFER, mWaterBufferSize * sizeof(WaterElement), mWaterBuffer.get(), GL_DYNAMIC_DRAW);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0u);
}

void RenderContext::RenderLand()
{
    // Use program
    glUseProgram(*mLandShaderProgram);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mLandTexture);

    // Bind land VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandBufferSize));

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Stop using program
    glUseProgram(0);
}

void RenderContext::RenderWater()
{
    // Use program
    glUseProgram(*mWaterShaderProgram);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mWaterTexture);

    // Bind water VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Describe InputTextureY
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterBufferSize));

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Stop using program
    glUseProgram(0);
}

void RenderContext::RenderEnd()
{
    glFlush();
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::UpdateOrthoMatrix()
{
    static constexpr float zFar = 1000.0f;
    static constexpr float zNear = 1.0f;

    // Calculate new matrix
    mOrthoMatrix[0][0] = 2.0f / mVisibleWorldWidth;
    mOrthoMatrix[1][1] = 2.0f / mVisibleWorldHeight;
    mOrthoMatrix[2][2] = -2.0f / (zFar - zNear);
    mOrthoMatrix[3][0] = -2.0f * mCamX / mVisibleWorldWidth;
    mOrthoMatrix[3][1] = -2.0f * mCamY / mVisibleWorldHeight;
    mOrthoMatrix[3][2] = -(zFar + zNear) / (zFar - zNear);
    mOrthoMatrix[3][3] = 1.0f;

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateOrthoMatrix(mOrthoMatrix);
    }

    // Set parameters in all programs

    glUseProgram(*mMatteWorldShaderProgram);
    glUniformMatrix4fv(mMatteWorldShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    glUseProgram(*mLandShaderProgram);
    glUniformMatrix4fv(mLandShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    glUseProgram(*mWaterShaderProgram);
    glUniformMatrix4fv(mWaterShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));
}

void RenderContext::UpdateVisibleWorldCoordinates()
{
    // Calculate new dimensions
    mVisibleWorldHeight = 2.0f * 70.0f / (mZoom + 0.001f);
    mVisibleWorldWidth = static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight) * mVisibleWorldHeight;
    mCanvasToVisibleWorldHeightRatio = static_cast<float>(mCanvasHeight) / mVisibleWorldHeight;

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateVisibleWorldCoordinates(
            mVisibleWorldHeight,
            mVisibleWorldWidth,
            mCanvasToVisibleWorldHeightRatio);
    }
}

void RenderContext::UpdateAmbientLightIntensity()
{
    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateAmbientLightIntensity(mAmbientLightIntensity);
    }

    // Set parameters in all programs

    glUseProgram(*mLandShaderProgram);
    glUniform1f(mLandShaderAmbientLightIntensityParameter, mAmbientLightIntensity);

    glUseProgram(*mWaterShaderProgram);
    glUniform1f(mWaterShaderAmbientLightIntensityParameter, mAmbientLightIntensity);

    glUseProgram(*mCloudShaderProgram);
    glUniform1f(mCloudShaderAmbientLightIntensityParameter, mAmbientLightIntensity);
}

void RenderContext::UpdateSeaWaterTransparency()
{
    // Set parameter in all programs

    glUseProgram(*mWaterShaderProgram);
    glUniform1f(mWaterShaderWaterTransparencyParameter, mSeaWaterTransparency);
}

void RenderContext::UpdateWaterLevelOfDetail()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->UpdateWaterLevelThreshold(mWaterLevelOfDetail);
    }
}