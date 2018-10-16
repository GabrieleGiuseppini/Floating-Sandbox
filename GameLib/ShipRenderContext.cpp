/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipRenderContext.h"

#include "GameException.h"
#include "GameMath.h"
#include "GameParameters.h"

namespace Render {

ShipRenderContext::ShipRenderContext(
    size_t pointCount,
    std::optional<ImageData> texture,
    ShaderManager<ShaderManagerTraits> & shaderManager,
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
    , mPointCount(pointCount)
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
    , mVectorArrowColor()
{
    GLuint tmpGLuint;

    // Clear errors
    glGetError();


    //
    // Create and initialize point VBOs
    //

    GLuint pointVBOs[5];
    glGenBuffers(5, pointVBOs);
    
    mPointPositionVBO = pointVBOs[0];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPositionVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec2f), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointPosition), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    CheckOpenGLError();

    mPointLightVBO = pointVBOs[1];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointLightVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointLight), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
    CheckOpenGLError();

    mPointWaterVBO = pointVBOs[2];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointWaterVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointWater), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
    CheckOpenGLError();

    mPointColorVBO = pointVBOs[3];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferData(GL_ARRAY_BUFFER, mPointCount * sizeof(vec3f), nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointColor), 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), (void*)(0));
    CheckOpenGLError();

    mPointElementTextureCoordinatesVBO = pointVBOs[4];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointElementTextureCoordinatesVBO);
    glBufferData(GL_ARRAY_BUFFER, mPointCount * sizeof(vec2f), nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointTextureCoordinates), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    CheckOpenGLError();

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
        CheckOpenGLError();

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(*texture));

        //
        // Configure texture
        //

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        CheckOpenGLError();

        // Set filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

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
    CheckOpenGLError();

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Make texture data
    unsigned char buf[] = {
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255,
        255, 253, 181, 255,     239, 16, 39, 255,       255, 253, 181,  255,
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255
    };

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    CheckOpenGLError();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);



    //
    // Initialize generic textures
    //

    mGenericTextureRenderPolygonVertexBuffer.reserve(1024);

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mGenericTextureRenderPolygonVertexVBO = tmpGLuint;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mGenericTextureRenderPolygonVertexVBO);
    CheckOpenGLError();

    // Describe buffers
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData1), 4, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)0);
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericTextureTextureCoordinates), 2, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)((2 + 2) * sizeof(float)));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData2), 4, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)((2 + 2 + 2) * sizeof(float)));
    CheckOpenGLError();


    //
    // Initialize vector field
    //

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mVectorArrowPointPositionVBO = tmpGLuint;



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

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();
    mShaderManager.SetProgramParameter<ProgramType::ShipStressedSprings, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::GenericTextures>();
    mShaderManager.SetProgramParameter<ProgramType::GenericTextures, ProgramParameterType::OrthoMatrix>(
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

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::GenericTextures>();
    mShaderManager.SetProgramParameter<ProgramType::GenericTextures, ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);
}

void ShipRenderContext::UpdateWaterLevelThreshold(float waterLevelOfDetail)
{
    // Transform: 0->1 == 2.0->0.01
    mWaterLevelThreshold = 2.0f + waterLevelOfDetail * (-2.0f + 0.01f);

    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterLevelThreshold>(
        mWaterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterLevelThreshold>(
        mWaterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterLevelThreshold>(
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
    vec3f const * restrict color,
    vec2f const * restrict textureCoordinates)
{
    // Upload colors
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec3f), color);
    CheckOpenGLError();

    if (!!mElementShipTexture)
    {
        // Upload texture coordinates
        glBindBuffer(GL_ARRAY_BUFFER, *mPointElementTextureCoordinatesVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec2f), textureCoordinates);
        CheckOpenGLError();
    }    
}

void ShipRenderContext::UploadPoints(
    vec2f const * restrict position,
    float const * restrict light,
    float const * restrict water)
{
    // Upload positions
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPositionVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec2f), position);
    CheckOpenGLError();

    // Upload light
    glBindBuffer(GL_ARRAY_BUFFER, *mPointLightVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(float), light);
    CheckOpenGLError();

    // Upload water
    glBindBuffer(GL_ARRAY_BUFFER, *mPointWaterVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(float), water);
    CheckOpenGLError();
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
        CheckOpenGLError();

        // Springs
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].springElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].springElementCount * sizeof(SpringElement), mConnectedComponents[c].springElementBuffer.get(), GL_STATIC_DRAW);
        CheckOpenGLError();

        // Ropes
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].ropeElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].ropeElementCount * sizeof(RopeElement), mConnectedComponents[c].ropeElementBuffer.get(), GL_STATIC_DRAW);
        CheckOpenGLError();

        // Triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mConnectedComponents[c].triangleElementVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mConnectedComponents[c].triangleElementCount * sizeof(TriangleElement), mConnectedComponents[c].triangleElementBuffer.get(), GL_STATIC_DRAW);
        CheckOpenGLError();
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
        CheckOpenGLError();
    }
}

void ShipRenderContext::UploadVectors(
    size_t count,
    vec2f const * restrict position,
    vec2f const * restrict vector,
    float lengthAdjustment,
    vec4f const & color)
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
    CheckOpenGLError();


    //
    // Store color
    //

    mVectorArrowColor = color;
}

void ShipRenderContext::RenderEnd()
{
    //
    // Upload Generic Textures data
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mGenericTextureRenderPolygonVertexVBO);
    glBufferData(GL_ARRAY_BUFFER, mGenericTextureRenderPolygonVertexBuffer.size() * sizeof(TextureRenderPolygonVertex), mGenericTextureRenderPolygonVertexBuffer.data(), GL_DYNAMIC_DRAW);
    CheckOpenGLError();

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
    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();

    // Set point size
    glPointSize(0.2f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.pointElementVBO);
    CheckOpenGLError();

    // Disable vertex attribute 0, as we don't use it
    glDisableVertexAttribArray(0);

    // Draw
    glDrawElements(GL_POINTS, static_cast<GLsizei>(1 * connectedComponent.pointElementCount), GL_UNSIGNED_INT, 0);
}

void ShipRenderContext::RenderSpringElements(
    ConnectedComponentData const & connectedComponent,
    bool withTexture)
{
    if (withTexture && !!mElementShipTexture)
    {
        // Use texture program
        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
        
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
        CheckOpenGLError();
    }
    else
    {
        // Use color program
        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    }

    // Set line size
    glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.springElementVBO);
    CheckOpenGLError();

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
        mShaderManager.ActivateProgram<ProgramType::ShipRopes>();

        // Set line size
        glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

        // Bind VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.ropeElementVBO);
        CheckOpenGLError();

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
        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
    }
    else
    {
        // Use color program
        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    }

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.triangleElementVBO);
    CheckOpenGLError();

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
        mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();
        
        // Set line size
        glLineWidth(0.1f * 2.0f * mCanvasToVisibleWorldHeightRatio);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mElementStressedSpringTexture);
        CheckOpenGLError();

        // Bind VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *connectedComponent.stressedSpringElementVBO);
        CheckOpenGLError();

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
        mShaderManager.ActivateProgram<ProgramType::GenericTextures>();

        // Disable vertex attribute 0, as we don't use it
        glDisableVertexAttribArray(0);

        // Draw all textures for this connected component
        for (size_t c = 0; c < connectedComponent.size(); ++c)
        {
            // Bind texture
            mTextureRenderManager.BindTexture(connectedComponent[c].frameId);

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
    // Use matte program
    mShaderManager.ActivateProgram<ProgramType::Matte>();

    // Set line size
    glLineWidth(0.5f);

    // Set vector color
    mShaderManager.SetProgramParameter<ProgramType::Matte, ProgramParameterType::MatteColor>(
        mVectorArrowColor.x,
        mVectorArrowColor.y,
        mVectorArrowColor.z,
        mVectorArrowColor.w);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowPointPositionVBO);
    CheckOpenGLError();

    // Describe buffer
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    CheckOpenGLError();

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);
    CheckOpenGLError();

    // Draw
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mVectorArrowPointPositionBuffer.size()));
}

}