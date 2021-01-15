/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "NotificationRenderContext.h"

#include <GameCore/GameWallClock.h>

#include <algorithm>

namespace Render {

float constexpr MarginScreen = 10.0f;
float constexpr MarginTopScreen = MarginScreen + 25.0f; // Consider menu bar

NotificationRenderContext::NotificationRenderContext(
    ResourceLocator const & resourceLocator,
    ShaderManager<ShaderManagerTraits> & shaderManager,
    GlobalRenderContext const & globalRenderContext)
    : mShaderManager(shaderManager)
    , mScreenToNdcX(0.0f) // Will be recalculated
    , mScreenToNdcY(0.0f) // Will be recalculated
	// Text
    , mTextNotificationContexts()
    , mTextVAO()
    , mAllocatedTextQuadVertexBufferSize(0)
    , mTextVBO()
    , mFonts()
    , mFontTextureAtlasMetadata()
    , mFontAtlasTextureHandle()
    // Texture notifications
    , mGenericLinearTextureAtlasMetadata(globalRenderContext.GetGenericLinearTextureAtlasMetadata())
    , mTextureNotifications()
    , mIsTextureNotificationDataDirty(false)
    , mTextureNotificationVAO()
    , mTextureNotificationVertexBuffer()
    , mTextureNotificationVBO()
    // Tool notifications
    , mHeatBlasterFlameVAO()
    , mHeatBlasterFlameVBO()
    , mHeatBlasterFlameShaderToRender()
    , mFireExtinguisherSprayVAO()
    , mFireExtinguisherSprayVBO()
    , mFireExtinguisherSprayShaderToRender()
{
    GLuint tmpGLuint;

    //
    // Load fonts
    //

    mFonts = Font::LoadAll(
        resourceLocator,
        [](float, ProgressMessageType) {});

    //
    // Initialize font texture atlas
    //

    std::vector<TextureFrame<FontTextureGroups>> fontTextures;
    for (size_t f = 0; f < mFonts.size(); ++f)
    {
        TextureFrameMetadata<FontTextureGroups> frameMetadata = TextureFrameMetadata<FontTextureGroups>(
            mFonts[f].Texture.Size,
            static_cast<float>(mFonts[f].Texture.Size.Width),
            static_cast<float>(mFonts[f].Texture.Size.Height),
            false,
            IntegralPoint(0, 0),
            vec2f::zero(),
            TextureFrameId<FontTextureGroups>(
                FontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)),
            std::to_string(f));

        fontTextures.emplace_back(
            frameMetadata,
            std::move(mFonts[f].Texture));
    }

    auto fontTextureAtlas = TextureAtlasBuilder<FontTextureGroups>::BuildAtlas(
        std::move(fontTextures),
        AtlasOptions::None);

    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

    glGenTextures(1, &tmpGLuint);
    mFontAtlasTextureHandle = tmpGLuint;

    glBindTexture(GL_TEXTURE_2D, *mFontAtlasTextureHandle);
    CheckOpenGLError();

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CheckOpenGLError();

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fontTextureAtlas.AtlasData.Size.Width, fontTextureAtlas.AtlasData.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fontTextureAtlas.AtlasData.Data.get());
    CheckOpenGLError();

    glBindTexture(GL_TEXTURE_2D, 0);

    // Store atlas metadata
    mFontTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<FontTextureGroups>>(fontTextureAtlas.Metadata);

    //
    // Initialize text notifications
    //

    // Set texture parameters
    mShaderManager.ActivateProgram<ProgramType::Text>();
    mShaderManager.SetTextureParameters<ProgramType::Text>();

    // Initialize VBO
    glGenBuffers(1, &tmpGLuint);
    mTextVBO = tmpGLuint;

    // Initialize VAO
    glGenVertexArrays(1, &tmpGLuint);
    mTextVAO = tmpGLuint;

    glBindVertexArray(*mTextVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mTextVBO);
    static_assert(sizeof(TextQuadVertex) == (4 + 1) * sizeof(float));
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Text1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Text1), 4, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Text2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Text2), 1, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void*)(4 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);

    // Initialize text notification contexts

    // Status text
    mTextNotificationContexts.emplace_back(
        mFonts[static_cast<size_t>(FontType::Font0)].Metadata,
        fontTextureAtlas.Metadata.GetFrameMetadata(TextureFrameId<FontTextureGroups>(FontTextureGroups::Font, 0)));

    // Notification text
    mTextNotificationContexts.emplace_back(
        mFonts[static_cast<size_t>(FontType::Font1)].Metadata,
        fontTextureAtlas.Metadata.GetFrameMetadata(TextureFrameId<FontTextureGroups>(FontTextureGroups::Font, 1)));

    //
    // Initialize texture notifications
    //

    {
        // Set texture parameters
        mShaderManager.ActivateProgram<ProgramType::TextureNotifications>();
        mShaderManager.SetTextureParameters<ProgramType::TextureNotifications>();

        // Initialize VAO
        glGenVertexArrays(1, &tmpGLuint);
        mTextureNotificationVAO = tmpGLuint;

        // Initialize VBO
        glGenBuffers(1, &tmpGLuint);
        mTextureNotificationVBO = tmpGLuint;

        // Describe vertex attributes
        glBindVertexArray(*mTextureNotificationVAO);
        glBindBuffer(GL_ARRAY_BUFFER, *mTextureNotificationVBO);
        static_assert(sizeof(TextureNotificationVertex) == (4 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::TextureNotification1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::TextureNotification1), 4, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::TextureNotification2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::TextureNotification2), 1, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void *)(4 * sizeof(float)));
        CheckOpenGLError();
        glBindVertexArray(0);
    }

    //
    // Initialize HeatBlaster flame
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mHeatBlasterFlameVAO = tmpGLuint;

        glBindVertexArray(*mHeatBlasterFlameVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mHeatBlasterFlameVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mHeatBlasterFlameVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame), 4, GL_FLOAT, GL_FALSE, sizeof(HeatBlasterFlameVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set noise in shader
        mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture2>();
        glBindTexture(GL_TEXTURE_2D, globalRenderContext.GetNoiseTextureOpenGLHandle(1));
        mShaderManager.ActivateProgram<ProgramType::HeatBlasterFlameCool>();
        mShaderManager.SetTextureParameters<ProgramType::HeatBlasterFlameCool>();
        mShaderManager.ActivateProgram<ProgramType::HeatBlasterFlameHeat>();
        mShaderManager.SetTextureParameters<ProgramType::HeatBlasterFlameHeat>();
    }

    //
    // Initialize Fire Extinguisher spray
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mFireExtinguisherSprayVAO = tmpGLuint;

        glBindVertexArray(*mFireExtinguisherSprayVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mFireExtinguisherSprayVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mFireExtinguisherSprayVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray), 4, GL_FLOAT, GL_FALSE, sizeof(FireExtinguisherSprayVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set noise in shader
        mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture2>();
        glBindTexture(GL_TEXTURE_2D, globalRenderContext.GetNoiseTextureOpenGLHandle(1));
        mShaderManager.ActivateProgram<ProgramType::FireExtinguisherSpray>();
        mShaderManager.SetTextureParameters<ProgramType::FireExtinguisherSpray>();
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void NotificationRenderContext::UploadStart()
{
    // Reset HeatBlaster flame, it's uploaded as needed
    mHeatBlasterFlameShaderToRender.reset();

    // Reset fire extinguisher spray, it's uploaded as needed
    mFireExtinguisherSprayShaderToRender.reset();
}

void NotificationRenderContext::UploadEnd()
{
    // Nop
}

void NotificationRenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
{
    if (renderParameters.IsViewDirty)
    {
        ApplyViewModelChanges(renderParameters);
    }

    if (renderParameters.IsCanvasSizeDirty)
    {
        ApplyCanvasSizeChanges(renderParameters);
    }

    if (renderParameters.IsEffectiveAmbientLightIntensityDirty)
    {
        ApplyEffectiveAmbientLightIntensityChanges(renderParameters);
    }
}

void NotificationRenderContext::RenderPrepare()
{
    RenderPrepareTextNotifications();

    RenderPrepareTextureNotifications();

    RenderPrepareHeatBlasterFlame();

    RenderPrepareFireExtinguisherSpray();
}

void NotificationRenderContext::RenderDraw()
{
    RenderDrawTextNotifications();

    RenderDrawTextureNotifications();

    RenderDrawHeatBlasterFlame();

    RenderDrawFireExtinguisherSpray();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationRenderContext::ApplyViewModelChanges(RenderParameters const & renderParameters)
{
    //
    // Update ortho matrix in all programs
    //

    constexpr float ZFar = 1000.0f;
    constexpr float ZNear = 1.0f;

    ViewModel::ProjectionMatrix globalOrthoMatrix;
    renderParameters.View.CalculateGlobalOrthoMatrix(ZFar, ZNear, globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::HeatBlasterFlameCool>();
    mShaderManager.SetProgramParameter<ProgramType::HeatBlasterFlameCool, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::HeatBlasterFlameHeat>();
    mShaderManager.SetProgramParameter<ProgramType::HeatBlasterFlameHeat, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::FireExtinguisherSpray>();
    mShaderManager.SetProgramParameter<ProgramType::FireExtinguisherSpray, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);
}

void NotificationRenderContext::ApplyCanvasSizeChanges(RenderParameters const & renderParameters)
{
    auto const & view = renderParameters.View;

    // Recalculate screen -> NDC conversion factors
    mScreenToNdcX = 2.0f / static_cast<float>(view.GetCanvasWidth());
    mScreenToNdcY = 2.0f / static_cast<float>(view.GetCanvasHeight());

    // Make sure we re-calculate (and re-upload) all text vertices
    // at the next iteration
    for (auto & tnc : mTextNotificationContexts)
    {
        tnc.AreTextLinesDirty = true;
    }

    // Make sure we re-calculate (and re-upload) all texture notification vertices
    // at the next iteration
    mIsTextureNotificationDataDirty = true;
}

void NotificationRenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters)
{
    float const lighteningStrength = Step(0.5f, 1.0f - renderParameters.EffectiveAmbientLightIntensity);

    // Set parameter in all programs
    mShaderManager.ActivateProgram<ProgramType::Text>();
    mShaderManager.SetProgramParameter<ProgramType::Text, ProgramParameterType::TextLighteningStrength>(
        lighteningStrength);
    mShaderManager.ActivateProgram<ProgramType::TextureNotifications>();
    mShaderManager.SetProgramParameter<ProgramType::TextureNotifications, ProgramParameterType::TextureLighteningStrength>(
        lighteningStrength);
}

void NotificationRenderContext::RenderPrepareTextNotifications()
{
    bool doNeedToUploadQuadVertexBuffers = false;

    for (auto & textNotificationContext : mTextNotificationContexts)
    {
        //
        // Re-generate vertex buffer if dirty
        //

        if (textNotificationContext.AreTextLinesDirty)
        {
            GenerateTextVertices(textNotificationContext);

            textNotificationContext.AreTextLinesDirty = false;

            // We need to re-upload the vertex buffers
            doNeedToUploadQuadVertexBuffers = true;
        }
    }

    if (doNeedToUploadQuadVertexBuffers)
    {
        //
        // Upload buffers
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mTextVBO);

        mAllocatedTextQuadVertexBufferSize = std::accumulate(
            mTextNotificationContexts.cbegin(),
            mTextNotificationContexts.cend(),
            size_t(0),
            [](size_t total, auto const & tnc) -> size_t
            {
                return total + tnc.TextQuadVertexBuffer.size();
            });

        glBufferData(
            GL_ARRAY_BUFFER,
            mAllocatedTextQuadVertexBufferSize * sizeof(TextQuadVertex),
            nullptr,
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        size_t start = 0;
        for (auto const & textNotificationContext : mTextNotificationContexts)
        {
            glBufferSubData(
                GL_ARRAY_BUFFER,
                start * sizeof(TextQuadVertex),
                textNotificationContext.TextQuadVertexBuffer.size() * sizeof(TextQuadVertex),
                textNotificationContext.TextQuadVertexBuffer.data());
            CheckOpenGLError();

            start += textNotificationContext.TextQuadVertexBuffer.size();
        }

        assert(start == mAllocatedTextQuadVertexBufferSize);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawTextNotifications()
{
    if (mAllocatedTextQuadVertexBufferSize > 0)
    {
        glBindVertexArray(*mTextVAO);

        // Activate texture unit
        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

        // Activate program
        mShaderManager.ActivateProgram<ProgramType::Text>();

        // Bind font texture
        glBindTexture(GL_TEXTURE_2D, *mFontAtlasTextureHandle);
        CheckOpenGLError();

        // Draw vertices
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mAllocatedTextQuadVertexBufferSize));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareTextureNotifications()
{
    //
    // Re-generate and upload vertex buffer if dirty
    //

    if (mIsTextureNotificationDataDirty)
    {
        //
        // Generate vertices
        //

        GenerateTextureNotificationVertices();

        //
        // Upload buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mTextureNotificationVBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            mTextureNotificationVertexBuffer.size() * sizeof(TextureNotificationVertex),
            mTextureNotificationVertexBuffer.data(),
            GL_STATIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mIsTextureNotificationDataDirty = false;
    }
}

void NotificationRenderContext::RenderDrawTextureNotifications()
{
    if (mTextureNotificationVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mTextureNotificationVAO);

        mShaderManager.ActivateProgram<ProgramType::TextureNotifications>();

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mTextureNotificationVertexBuffer.size()));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareHeatBlasterFlame()
{
    if (mHeatBlasterFlameShaderToRender.has_value())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mHeatBlasterFlameVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(HeatBlasterFlameVertex) * mHeatBlasterFlameVertexBuffer.size(),
            mHeatBlasterFlameVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawHeatBlasterFlame()
{
    if (mHeatBlasterFlameShaderToRender.has_value())
    {
        glBindVertexArray(*mHeatBlasterFlameVAO);

        mShaderManager.ActivateProgram(*mHeatBlasterFlameShaderToRender);

        // Set time parameter
        mShaderManager.SetProgramParameter<ProgramParameterType::Time>(
            *mHeatBlasterFlameShaderToRender,
            GameWallClock::GetInstance().ContinuousNowAsFloat());

        assert((mHeatBlasterFlameVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mHeatBlasterFlameVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareFireExtinguisherSpray()
{
    if (mFireExtinguisherSprayShaderToRender.has_value())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mFireExtinguisherSprayVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(FireExtinguisherSprayVertex) * mFireExtinguisherSprayVertexBuffer.size(),
            mFireExtinguisherSprayVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawFireExtinguisherSpray()
{
    if (mFireExtinguisherSprayShaderToRender.has_value())
    {
        glBindVertexArray(*mFireExtinguisherSprayVAO);

        mShaderManager.ActivateProgram(*mFireExtinguisherSprayShaderToRender);

        // Set time parameter
        mShaderManager.SetProgramParameter<ProgramParameterType::Time>(
            *mFireExtinguisherSprayShaderToRender,
            GameWallClock::GetInstance().ContinuousNowAsFloat());

        // Draw
        assert((mFireExtinguisherSprayVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mFireExtinguisherSprayVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::GenerateTextVertices(TextNotificationContext & context)
{
    FontMetadata const & fontMetadata = context.NotificationFontMetadata;

    context.TextQuadVertexBuffer.clear();

    for (auto const & textLine : context.TextLines)
    {
        //
        // Calculate line position in NDC coordinates
        //

        vec2f linePositionNdc( // Top-left of quads; start with offset
            textLine.ScreenOffset.x * static_cast<float>(fontMetadata.GetCharScreenWidth()) * mScreenToNdcX,
            -textLine.ScreenOffset.y * static_cast<float>(fontMetadata.GetLineScreenHeight()) * mScreenToNdcY);

        switch (textLine.Anchor)
        {
            case AnchorPositionType::BottomLeft:
            {
                linePositionNdc += vec2f(
                    -1.f + MarginScreen * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(fontMetadata.GetLineScreenHeight())) * mScreenToNdcY);

                break;
            }

            case AnchorPositionType::BottomRight:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    1.f - (MarginScreen + static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(lineExtent.Height)) * mScreenToNdcY);

                break;
            }

            case AnchorPositionType::TopLeft:
            {
                linePositionNdc += vec2f(
                    -1.f + MarginScreen * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }

            case AnchorPositionType::TopRight:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    1.f - (MarginScreen + static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }
        }

        //
        // Emit quads for this line
        //

        fontMetadata.EmitQuadVertices(
            textLine.Text.c_str(),
            textLine.Text.length(),
            linePositionNdc,
            textLine.Alpha,
            mScreenToNdcX,
            mScreenToNdcY,
            // TODOHERE: pass from atlas: ndc origin and ndc width/height
            context.TextQuadVertexBuffer);
    }
}

void NotificationRenderContext::GenerateTextureNotificationVertices()
{
    mTextureNotificationVertexBuffer.clear();

    for (auto const & textureNotification : mTextureNotifications)
    {
        //
        // Populate the texture quad
        //

        TextureAtlasFrameMetadata<GenericLinearTextureGroups> const & frame =
            mGenericLinearTextureAtlasMetadata.GetFrameMetadata(textureNotification.FrameId);

        ImageSize const & frameSize = frame.FrameMetadata.Size;

        vec2f quadTopLeft( // Start with offset
            textureNotification.ScreenOffset.x * static_cast<float>(frameSize.Width) * mScreenToNdcX,
            -textureNotification.ScreenOffset.y * static_cast<float>(frameSize.Height) * mScreenToNdcY);

        switch (textureNotification.Anchor)
        {
            case AnchorPositionType::BottomLeft:
            {
                quadTopLeft += vec2f(
                    -1.f + MarginScreen * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(frameSize.Height)) * mScreenToNdcY);

                break;
            }

            case AnchorPositionType::BottomRight:
            {
                quadTopLeft += vec2f(
                    1.f - (MarginScreen + static_cast<float>(frameSize.Width)) * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(frameSize.Height)) * mScreenToNdcY);

                break;
            }

            case AnchorPositionType::TopLeft:
            {
                quadTopLeft += vec2f(
                    -1.f + MarginScreen * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }

            case AnchorPositionType::TopRight:
            {
                quadTopLeft += vec2f(
                    1.f - (MarginScreen + static_cast<float>(frameSize.Width)) * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }
        }

        vec2f quadBottomRight = quadTopLeft + vec2f(
            static_cast<float>(frameSize.Width) * mScreenToNdcX,
            -static_cast<float>(frameSize.Height) * mScreenToNdcY);

        // Append vertices - two triangles

        // Triangle 1

        // Top-left
        mTextureNotificationVertexBuffer.emplace_back(
            quadTopLeft,
            vec2f(frame.TextureCoordinatesBottomLeft.x, frame.TextureCoordinatesTopRight.y),
            textureNotification.Alpha);

        // Top-Right
        mTextureNotificationVertexBuffer.emplace_back(
            vec2f(quadBottomRight.x, quadTopLeft.y),
            frame.TextureCoordinatesTopRight,
            textureNotification.Alpha);

        // Bottom-left
        mTextureNotificationVertexBuffer.emplace_back(
            vec2f(quadTopLeft.x, quadBottomRight.y),
            frame.TextureCoordinatesBottomLeft,
            textureNotification.Alpha);

        // Triangle 2

        // Top-Right
        mTextureNotificationVertexBuffer.emplace_back(
            vec2f(quadBottomRight.x, quadTopLeft.y),
            frame.TextureCoordinatesTopRight,
            textureNotification.Alpha);

        // Bottom-left
        mTextureNotificationVertexBuffer.emplace_back(
            vec2f(quadTopLeft.x, quadBottomRight.y),
            frame.TextureCoordinatesBottomLeft,
            textureNotification.Alpha);

        // Bottom-right
        mTextureNotificationVertexBuffer.emplace_back(
            quadBottomRight,
            vec2f(frame.TextureCoordinatesTopRight.x, frame.TextureCoordinatesBottomLeft.y),
            textureNotification.Alpha);
    }
}

}