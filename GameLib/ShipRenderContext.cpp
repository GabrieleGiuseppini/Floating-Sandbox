/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipRenderContext.h"

#include "GameException.h"
#include "GameMath.h"
#include "GameParameters.h"

ShipRenderContext::ShipRenderContext(    
    std::optional<ImageData> texture,
    ShaderManager<Render::ShaderManagerTraits> & shaderManager,
    TextureRenderManager const & textureRenderManager,
    float const(&orthoMatrix)[4][4],
    float visibleWorldHeight,
    float visibleWorldWidth,
    float canvasToVisibleWorldHeightRatio,
    float ambientLightIntensity,
    float waterLevelOfDetail,
    ShipRenderMode shipRenderMode,
    VectorFieldRenderMode vectorFieldRenderMode,
    bool showStressedSprings)
    : mShaderManager(shaderManager)
    // Parameters    
    , mCanvasToVisibleWorldHeightRatio(0)
    , mAmbientLightIntensity(0.0f) // Set later
    , mWaterLevelThreshold(0.0f)
    , mShipRenderMode(ShipRenderMode::Structure)
    , mVectorFieldRenderMode(VectorFieldRenderMode::None)
    , mShowStressedSprings(false)
    // Textures
    , mElementShipTexture()
    , mElementStressedSpringTexture()
    // Points
    , mPointCount(0u)
    , mPointPositionVBO()
    , mPointLightVBO()
    , mPointWaterVBO()
    , mPointColorVBO()
    , mPointElementTextureCoordinatesVBO()
    // Generic Textures
    , mTextureRenderManager(textureRenderManager)
    , mConnectedComponentGenericTextureInfos()
    , mGenericTextureRenderPolygonVertexBuffer()
    , mGenericTextureRenderPolygonVertexVBO()
    // Connected components
    , mConnectedComponentsMaxSizes()
    , mConnectedComponents()
    // Vectors
    , mVectorArrowPointPositionBuffer()
    , mVectorArrowPointPositionVBO()
{
    GLuint tmpGLuint;
    GLenum glError;

    // Clear errors
    glError = glGetError();


    //
    // Create and initialize point VBOs
    //

    GLuint pointVBOs[5];
    glGenBuffers(5, pointVBOs);

    mPointPositionVBO = pointVBOs[0];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPositionVBO);
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::ShipPointPosition), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));

    mPointLightVBO = pointVBOs[1];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointLightVBO);
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::ShipPointLight), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));

    mPointWaterVBO = pointVBOs[2];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointWaterVBO);
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::ShipPointWater), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));

    mPointColorVBO = pointVBOs[3];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::ShipPointColor), 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), (void*)(0));

    mPointElementTextureCoordinatesVBO = pointVBOs[4];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointElementTextureCoordinatesVBO);
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::ShipPointTextureCoordinates), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Create and upload ship texture, if present
    //

    if (!!texture)
    {
        glGenTextures(1, &tmpGLuint);
        mElementShipTexture = tmpGLuint;

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
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
    // Initialize generic textures
    //

    // Create VBO
    GLuint genericTextureRenderPolygonVertexVBO;
    glGenBuffers(1, &genericTextureRenderPolygonVertexVBO);
    mGenericTextureRenderPolygonVertexVBO = genericTextureRenderPolygonVertexVBO;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mGenericTextureRenderPolygonVertexVBO);

    // Describe buffers
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::GenericTexturePosition), 2, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)0);
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::GenericTextureCoordinates), 2, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)(2 * sizeof(float)));
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::GenericTextureAmbientLightSensitivity), 1, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)((2 + 2) * sizeof(float)));

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Initialize vector field
    //

    // Create VBO
    GLuint vectorArrowPointPositionVBO;
    glGenBuffers(1, &vectorArrowPointPositionVBO);
    mVectorArrowPointPositionVBO = vectorArrowPointPositionVBO;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowPointPositionVBO);

    // Describe buffers
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::ShipVectorPosition), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);


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
    UpdateShipRenderMode(shipRenderMode);
    UpdateVectorFieldRenderMode(vectorFieldRenderMode);
    UpdateShowStressedSprings(showStressedSprings);
}

ShipRenderContext::~ShipRenderContext()
{
}

void ShipRenderContext::UpdateOrthoMatrix(float const(&orthoMatrix)[4][4])
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipTrianglesColor, Render::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipTrianglesTexture, Render::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<Render::ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipRopes, Render::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<Render::ProgramType::ShipStressedSprings>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipStressedSprings, Render::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<Render::ProgramType::GenericTextures>();
    mShaderManager.SetProgramParameter<Render::ProgramType::GenericTextures, Render::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<Render::ProgramType::VectorArrows>();
    mShaderManager.SetProgramParameter<Render::ProgramType::VectorArrows, Render::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);
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

    mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipTrianglesColor, Render::ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);

    mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipTrianglesTexture, Render::ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);

    mShaderManager.ActivateProgram<Render::ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipRopes, Render::ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);

    mShaderManager.ActivateProgram<Render::ProgramType::GenericTextures>();
    mShaderManager.SetProgramParameter<Render::ProgramType::GenericTextures, Render::ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);
}

void ShipRenderContext::UpdateWaterLevelThreshold(float waterLevelOfDetail)
{
    // Transform: 0->1 == 2.0->0.01
    mWaterLevelThreshold = 2.0f + waterLevelOfDetail * (-2.0f + 0.01f);

    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipTrianglesColor, Render::ProgramParameterType::WaterLevelThreshold>(
        mWaterLevelThreshold);

    mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipTrianglesTexture, Render::ProgramParameterType::WaterLevelThreshold>(
        mWaterLevelThreshold);

    mShaderManager.ActivateProgram<Render::ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<Render::ProgramType::ShipRopes, Render::ProgramParameterType::WaterLevelThreshold>(
        mWaterLevelThreshold);
}

void ShipRenderContext::UpdateShipRenderMode(ShipRenderMode shipRenderMode)
{
    mShipRenderMode = shipRenderMode;
}

void ShipRenderContext::UpdateVectorFieldRenderMode(VectorFieldRenderMode vectorFieldRenderMode)
{
    mVectorFieldRenderMode = vectorFieldRenderMode;
}

void ShipRenderContext::UpdateShowStressedSprings(bool showStressedSprings)
{
    mShowStressedSprings = showStressedSprings;
}

//////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::RenderStart(std::vector<std::size_t> const & connectedComponentsMaxSizes)
{
    // Store connected component max sizes
    mConnectedComponentsMaxSizes = connectedComponentsMaxSizes;

    //
    // Reset generic textures 
    //

    mConnectedComponentGenericTextureInfos.clear();
    mConnectedComponentGenericTextureInfos.resize(connectedComponentsMaxSizes.size());

    mGenericTextureRenderPolygonVertexBuffer.clear();
    mGenericTextureRenderPolygonVertexBuffer.reserve(4 * 200);
}

void ShipRenderContext::UploadPointImmutableGraphicalAttributes(
    size_t count,
    vec3f const * restrict color,
    vec2f const * restrict textureCoordinates)
{
    // Upload colors
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(vec3f), color, GL_STATIC_DRAW);

    if (!!mElementShipTexture)
    {
        // Upload texture coordinates
        glBindBuffer(GL_ARRAY_BUFFER, *mPointElementTextureCoordinatesVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(vec2f), textureCoordinates, GL_STATIC_DRAW);
    }    

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
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(vec2f), position, GL_DYNAMIC_DRAW);

    // Upload lights
    glBindBuffer(GL_ARRAY_BUFFER, *mPointLightVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), light, GL_DYNAMIC_DRAW);

    // Upload waters
    glBindBuffer(GL_ARRAY_BUFFER, *mPointWaterVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), water, GL_DYNAMIC_DRAW);
}

void ShipRenderContext::UploadElementsStart()
{
    GLuint elementVBO;

    if (mConnectedComponentsMaxSizes.size() != mConnectedComponents.size())
    {
        // A change in the number of connected components, nuke everything
        mConnectedComponents.clear();
        mConnectedComponents.resize(mConnectedComponentsMaxSizes.size());
    }
    
    for (size_t c = 0; c < mConnectedComponentsMaxSizes.size(); ++c)
    {
        //
        // Prepare point elements
        //

        // Max # of points = number of points
        size_t maxConnectedComponentPoints = mConnectedComponentsMaxSizes[c];
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

        size_t maxConnectedComponentSprings = mConnectedComponentsMaxSizes[c] * GameParameters::MaxSpringsPerPoint;
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

        size_t maxConnectedComponentTriangles = mConnectedComponentsMaxSizes[c] * GameParameters::MaxTrianglesPerPoint;
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
    }
}

void ShipRenderContext::UploadElementsEnd()
{
    //
    // Upload all elements, except for stressed springs
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
    glBufferData(GL_ARRAY_BUFFER, mVectorArrowPointPositionBuffer.size() * sizeof(vec2f), mVectorArrowPointPositionBuffer.data(), GL_DYNAMIC_DRAW);


    //
    // Set color parameter
    //

    mShaderManager.ActivateProgram<Render::ProgramType::VectorArrows>();
    mShaderManager.SetProgramParameter<Render::ProgramType::VectorArrows, Render::ProgramParameterType::MatteColor>(
        color.x, 
        color.y, 
        color.z);
}

void ShipRenderContext::RenderEnd()
{
    //
    // Upload Generic Textures data
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mGenericTextureRenderPolygonVertexVBO);
    glBufferData(GL_ARRAY_BUFFER, mGenericTextureRenderPolygonVertexBuffer.size() * sizeof(TextureRenderPolygonVertex), mGenericTextureRenderPolygonVertexBuffer.data(), GL_DYNAMIC_DRAW);


    //
    // Process all connected components, from first to last, and draw all elements
    //

    for (size_t c = 0; c < mConnectedComponents.size(); ++c)
    {
        //
        // Draw points
        //

        if (mShipRenderMode == ShipRenderMode::Points)
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

        if (mShipRenderMode == ShipRenderMode::Springs
            || mShipRenderMode == ShipRenderMode::Structure
            || mShipRenderMode == ShipRenderMode::Texture)
        {
            RenderSpringElements(
                mConnectedComponents[c],
                mShipRenderMode == ShipRenderMode::Texture);
        }


        //
        // Draw ropes now if RenderMode is:
        // - Springs
        // - Texture (so rope endpoints are hidden behind texture, looks better)
        //

        if (mShipRenderMode == ShipRenderMode::Springs
            || mShipRenderMode == ShipRenderMode::Texture)
        {
            RenderRopeElements(mConnectedComponents[c]);
        }


        //
        // Draw triangles
        //

        if (mShipRenderMode == ShipRenderMode::Structure
            || mShipRenderMode == ShipRenderMode::Texture)
        {
            RenderTriangleElements(
                mConnectedComponents[c],
                mShipRenderMode == ShipRenderMode::Texture);
        }


        //
        // Draw ropes now if RenderMode is Structure (so rope endpoints on the structure are visible)
        //

        if (mShipRenderMode == ShipRenderMode::Structure)
        {
            RenderRopeElements(mConnectedComponents[c]);
        }


        //
        // Draw stressed springs
        //

        if (mShowStressedSprings)
        {
            RenderStressedSpringElements(mConnectedComponents[c]);
        }


        //
        // Draw Generic textures
        //

        RenderGenericTextures(mConnectedComponentGenericTextureInfos[c]);
    }


    //
    // Render vectors, if we're asked to
    //

    if (mVectorFieldRenderMode != VectorFieldRenderMode::None)
    {
        RenderVectors();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::RenderPointElements(ConnectedComponentData const & connectedComponent)
{
    // Use color program
    mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesColor>();

    // Set point size
    glPointSize(0.2f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.pointElementVBO);

    // Disable vertex attribute 0, as we don't use it
    glDisableVertexAttribArray(0);

    // Draw
    glDrawElements(GL_POINTS, static_cast<GLsizei>(1 * connectedComponent.pointElementCount), GL_UNSIGNED_INT, 0);}

void ShipRenderContext::RenderSpringElements(
    ConnectedComponentData const & connectedComponent,
    bool withTexture)
{
    if (withTexture && !!mElementShipTexture)
    {
        // Use texture program
        mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesTexture>();
        
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
    }
    else
    {
        // Use color program
        mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesColor>();
    }

    // Set line size
    glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.springElementVBO);

    // Disable vertex attribute 0, as we don't use it
    glDisableVertexAttribArray(0);

    // Draw
    glDrawElements(GL_LINES, static_cast<GLsizei>(2 * connectedComponent.springElementCount), GL_UNSIGNED_INT, 0);
}

void ShipRenderContext::RenderRopeElements(ConnectedComponentData const & connectedComponent)
{
    if (connectedComponent.ropeElementCount > 0)
    {
        // Use rope program
        mShaderManager.ActivateProgram<Render::ProgramType::ShipRopes>();

        // Set line size
        glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

        // Bind VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.ropeElementVBO);

        // Disable vertex attribute 0, as we don't use it
        glDisableVertexAttribArray(0);

        // Draw
        glDrawElements(GL_LINES, static_cast<GLsizei>(2 * connectedComponent.ropeElementCount), GL_UNSIGNED_INT, 0);
    }
}

void ShipRenderContext::RenderTriangleElements(
    ConnectedComponentData const & connectedComponent,
    bool withTexture)
{
    if (withTexture && !!mElementShipTexture)
    {
        // Use texture program
        mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesTexture>();

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
    }
    else
    {
        // Use color program
        mShaderManager.ActivateProgram<Render::ProgramType::ShipTrianglesColor>();
    }

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.triangleElementVBO);

    // Disable vertex attribute 0, as we don't use it
    glDisableVertexAttribArray(0);

    // Draw
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(3 * connectedComponent.triangleElementCount), GL_UNSIGNED_INT, 0);
}

void ShipRenderContext::RenderStressedSpringElements(ConnectedComponentData const & connectedComponent)
{
    if (connectedComponent.stressedSpringElementCount > 0)
    {
        // Use program
        mShaderManager.ActivateProgram<Render::ProgramType::ShipStressedSprings>();
        // Set line size
        glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementStressedSpringTexture);

        // Bind VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.stressedSpringElementVBO);

        // Disable vertex attribute 0, as we don't use it
        glDisableVertexAttribArray(0);

        // Draw
        glDrawElements(GL_LINES, static_cast<GLsizei>(2 * connectedComponent.stressedSpringElementCount), GL_UNSIGNED_INT, 0);
    }
}

void ShipRenderContext::RenderGenericTextures(std::vector<GenericTextureInfo> const & connectedComponent)
{
    if (!connectedComponent.empty())
    {
        // Use program
        mShaderManager.ActivateProgram<Render::ProgramType::GenericTextures>();

        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mGenericTextureRenderPolygonVertexVBO);

        // Disable vertex attribute 0, as we don't use it
        glDisableVertexAttribArray(0);

        // Draw all textures for this connected component
        for (size_t c = 0; c < connectedComponent.size(); ++c)
        {
            // Bind texture
            glBindTexture(
                GL_TEXTURE_2D,
                mTextureRenderManager.GetOpenGLHandle(connectedComponent[c].frameId));

            // Draw polygon
            glDrawArrays(
                GL_TRIANGLE_STRIP,
                static_cast<GLint>(connectedComponent[c].polygonIndex),
                4);
        }    
    }
}

void ShipRenderContext::RenderVectors()
{
    // Use vector arrow program
    mShaderManager.ActivateProgram<Render::ProgramType::VectorArrows>();

    // Set line size
    glLineWidth(0.5f);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowPointPositionVBO);

    // Disable vertex attribute 0, as we don't use it
    glDisableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mVectorArrowPointPositionBuffer.size()));
}