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
    , mFontTextureAtlasMetadata()
    , mTextNotificationTypeContexts()
    , mTextVAO()
    , mCurrentTextQuadVertexBufferSize(0)
    , mAllocatedTextQuadVertexBufferSize(0)
    , mTextVBO()
    , mFontAtlasTextureHandle()
    // Texture notifications
    , mGenericLinearTextureAtlasMetadata(globalRenderContext.GetGenericLinearTextureAtlasMetadata())
    , mTextureNotifications()
    , mIsTextureNotificationDataDirty(false)
    , mTextureNotificationVAO()
    , mTextureNotificationVertexBuffer()
    , mTextureNotificationVBO()
    // Physics probe panel
    , mPhysicsProbePanelVAO()
    , mPhysicsProbePanelVertexBuffer()
    , mIsPhysicsProbePanelVertexBufferDirty(false)
    , mPhysicsProbePanelVBO()
    , mPhysicsProbePanelNdcDimensions(vec2f::zero()) // Will be recalculated
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

    std::vector<Font> fonts = Font::LoadAll(
        resourceLocator,
        [](float, ProgressMessageType) {});

    //
    // Build font texture atlas
    //

    std::vector<TextureFrame<FontTextureGroups>> fontTextures;

    for (size_t f = 0; f < fonts.size(); ++f)
    {
        TextureFrameMetadata<FontTextureGroups> frameMetadata = TextureFrameMetadata<FontTextureGroups>(
            fonts[f].Texture.Size,
            static_cast<float>(fonts[f].Texture.Size.Width),
            static_cast<float>(fonts[f].Texture.Size.Height),
            false,
            IntegralPoint(0, 0),
            vec2f::zero(),
            TextureFrameId<FontTextureGroups>(
                FontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)),
            std::to_string(f));

        fontTextures.emplace_back(
            frameMetadata,
            std::move(fonts[f].Texture));
    }

    auto fontTextureAtlas = TextureAtlasBuilder<FontTextureGroups>::BuildAtlas(
        std::move(fontTextures),
        AtlasOptions::None);

    LogMessage("Font texture atlas size: ", fontTextureAtlas.AtlasData.Size.ToString());

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

    // Upload texture atlas
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fontTextureAtlas.AtlasData.Size.Width, fontTextureAtlas.AtlasData.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fontTextureAtlas.AtlasData.Data.get());
    CheckOpenGLError();

    glBindTexture(GL_TEXTURE_2D, 0);

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

    // Initialize font texture atlas metadata
    for (size_t f = 0; f < fonts.size(); ++f)
    {
        auto const & fontTextureFrameMetadata = fontTextureAtlas.Metadata.GetFrameMetadata(
            TextureFrameId<FontTextureGroups>(
                FontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)));

        // Dimensions of a cell of this font, in the atlas' texture space coordinates
        float const fontCellWidthAtlasTextureSpace = static_cast<float>(fonts[f].Metadata.GetCellScreenWidth()) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().Width);
        float const fontCellHeightAtlasTextureSpace = static_cast<float>(fonts[f].Metadata.GetCellScreenHeight()) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().Height);

        // Coordinates for each character
        std::array<vec2f, 256> GlyphTextureBottomLefts;
        std::array<vec2f, 256> GlyphTextureTopRights;
        for (int c = 0; c < 256; ++c)
        {
            // Texture-space left x
            int const glyphTextureCol = (c - FontMetadata::BaseCharacter) % fonts[f].Metadata.GetGlyphsPerTextureRow();
            float const glyphLeftAtlasTextureSpace =
                fontTextureFrameMetadata.TextureCoordinatesBottomLeft.x // Includes dead-center dx already
                + static_cast<float>(glyphTextureCol) * fontCellWidthAtlasTextureSpace;

            // Texture-space right x
            float const glyphRightAtlasTextureSpace =
                glyphLeftAtlasTextureSpace
                + static_cast<float>(fonts[f].Metadata.GetGlyphScreenWidth(c) - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().Width);

            // Texture-space top y
            // Note: font texture is flipped vertically (top of character is at lower V coordinates)
            int const glyphTextureRow = (c - FontMetadata::BaseCharacter) / fonts[f].Metadata.GetGlyphsPerTextureRow();
            float const glyphTopAtlasTextureSpace =
                fontTextureFrameMetadata.TextureCoordinatesBottomLeft.y // Includes dead-center dx already
                + static_cast<float>(glyphTextureRow) * fontCellHeightAtlasTextureSpace;

            float const glyphBottomAtlasTextureSpace =
                glyphTopAtlasTextureSpace
                + static_cast<float>(fonts[f].Metadata.GetGlyphScreenHeight(c) - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().Height);

            GlyphTextureBottomLefts[c] = vec2f(glyphLeftAtlasTextureSpace, glyphBottomAtlasTextureSpace);
            GlyphTextureTopRights[c] = vec2f(glyphRightAtlasTextureSpace, glyphTopAtlasTextureSpace);
        }

        // Store
        mFontTextureAtlasMetadata.emplace_back(
            vec2f(fontCellWidthAtlasTextureSpace, fontCellHeightAtlasTextureSpace),
            GlyphTextureBottomLefts,
            GlyphTextureTopRights,
            fonts[f].Metadata);
    }

    // Initialize text notification contexts for each type of notification
    {
        // Status text
        static_assert(static_cast<size_t>(TextNotificationType::StatusText) == 0);
        mTextNotificationTypeContexts.emplace_back(mFontTextureAtlasMetadata[static_cast<size_t>(FontType::Font0)]);

        // Notification text
        static_assert(static_cast<size_t>(TextNotificationType::NotificationText) == 1);
        mTextNotificationTypeContexts.emplace_back(mFontTextureAtlasMetadata[static_cast<size_t>(FontType::Font1)]);

        // Physics probe reading
        static_assert(static_cast<size_t>(TextNotificationType::PhysicsProbeReading) == 2);
        mTextNotificationTypeContexts.emplace_back(mFontTextureAtlasMetadata[static_cast<size_t>(FontType::Font2)]);
    }

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
    // Initialize Physics probe panel
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mPhysicsProbePanelVAO = tmpGLuint;

        glBindVertexArray(*mPhysicsProbePanelVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mPhysicsProbePanelVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mPhysicsProbePanelVBO);
        static_assert(sizeof(PhysicsProbePanelVertex) == 7 * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel1), 4, GL_FLOAT, GL_FALSE, sizeof(PhysicsProbePanelVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel2), 3, GL_FLOAT, GL_FALSE, sizeof(PhysicsProbePanelVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set noise in shader
        mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture2>();
        glBindTexture(GL_TEXTURE_2D, globalRenderContext.GetNoiseTextureOpenGLHandle(1));
        mShaderManager.ActivateProgram<ProgramType::PhysicsProbePanel>();
        mShaderManager.SetTextureParameters<ProgramType::PhysicsProbePanel>();
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

    RenderPreparePhysicsProbePanel();

    RenderPrepareHeatBlasterFlame();

    RenderPrepareFireExtinguisherSpray();
}

void NotificationRenderContext::RenderDraw()
{
    RenderDrawPhysicsProbePanel(); // Draw panel first

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
    for (auto & tntc : mTextNotificationTypeContexts)
    {
        tntc.AreTextLinesDirty = true;
    }

    // Make sure we re-calculate (and re-upload) all texture notification vertices
    // at the next iteration
    mIsTextureNotificationDataDirty = true;

    // Recalculate NDC dimensions of physics probe panel
    auto const & atlasFrame = mGenericLinearTextureAtlasMetadata.GetFrameMetadata(TextureFrameId<GenericLinearTextureGroups>(GenericLinearTextureGroups::PhysicsProbePanel, 0));
    mPhysicsProbePanelNdcDimensions = vec2f(
        static_cast<float>(atlasFrame.FrameMetadata.Size.Width) * mScreenToNdcX,
        static_cast<float>(atlasFrame.FrameMetadata.Size.Height) * mScreenToNdcY);

    // Set parameters
    mShaderManager.ActivateProgram<ProgramType::PhysicsProbePanel>();
    mShaderManager.SetProgramParameter<ProgramType::PhysicsProbePanel, ProgramParameterType::WidthNdc>(
        mPhysicsProbePanelNdcDimensions.x);
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
    //
    // Check whether we need to re-generate - and thus re-upload - quad vertex buffers
    //

    bool doNeedToUploadQuadVertexBuffers = false;

    for (auto & textNotificationTypeContext : mTextNotificationTypeContexts)
    {
        if (textNotificationTypeContext.AreTextLinesDirty)
        {
            // Re-generated quad vertices for this notification type
            GenerateTextVertices(textNotificationTypeContext);

            textNotificationTypeContext.AreTextLinesDirty = false;

            // We need to re-upload the vertex buffers
            doNeedToUploadQuadVertexBuffers = true;
        }
    }

    if (doNeedToUploadQuadVertexBuffers)
    {
        //
        // Re-upload whole buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mTextVBO);

        // Calculate total buffer size
        mCurrentTextQuadVertexBufferSize = std::accumulate(
            mTextNotificationTypeContexts.cbegin(),
            mTextNotificationTypeContexts.cend(),
            size_t(0),
            [](size_t total, auto const & tntc) -> size_t
            {
                return total + tntc.TextQuadVertexBuffer.size();
            });

        if (mCurrentTextQuadVertexBufferSize > mAllocatedTextQuadVertexBufferSize)
        {
            // Allocate buffer
            glBufferData(
                GL_ARRAY_BUFFER,
                mCurrentTextQuadVertexBufferSize * sizeof(TextQuadVertex),
                nullptr,
                GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mAllocatedTextQuadVertexBufferSize = mCurrentTextQuadVertexBufferSize;
        }

        // Upload buffer in chunks
        size_t start = 0;
        for (auto const & textNotificationTypeContext : mTextNotificationTypeContexts)
        {
            glBufferSubData(
                GL_ARRAY_BUFFER,
                start * sizeof(TextQuadVertex),
                textNotificationTypeContext.TextQuadVertexBuffer.size() * sizeof(TextQuadVertex),
                textNotificationTypeContext.TextQuadVertexBuffer.data());
            CheckOpenGLError();

            start += textNotificationTypeContext.TextQuadVertexBuffer.size();
        }

        assert(start == mCurrentTextQuadVertexBufferSize);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawTextNotifications()
{
    if (mCurrentTextQuadVertexBufferSize > 0)
    {
        glBindVertexArray(*mTextVAO);

        // Activate texture unit
        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

        // Bind font atlas texture
        glBindTexture(GL_TEXTURE_2D, *mFontAtlasTextureHandle);
        CheckOpenGLError();

        // Activate program
        mShaderManager.ActivateProgram<ProgramType::Text>();

        // Draw vertices
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mCurrentTextQuadVertexBufferSize));
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

void NotificationRenderContext::RenderPreparePhysicsProbePanel()
{
    if (mIsPhysicsProbePanelVertexBufferDirty)
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mPhysicsProbePanelVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(PhysicsProbePanelVertex) * mPhysicsProbePanelVertexBuffer.size(),
            mPhysicsProbePanelVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mIsPhysicsProbePanelVertexBufferDirty = false;
    }
}

void NotificationRenderContext::RenderDrawPhysicsProbePanel()
{
    if (mPhysicsProbePanelVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mPhysicsProbePanelVAO);

        mShaderManager.ActivateProgram<ProgramType::PhysicsProbePanel>();

        assert((mPhysicsProbePanelVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mPhysicsProbePanelVertexBuffer.size()));

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

void NotificationRenderContext::GenerateTextVertices(TextNotificationTypeContext & context) const
{
    FontTextureAtlasMetadata const & fontTextureAtlasMetadata = context.NotificationFontTextureAtlasMetadata;
    FontMetadata const & fontMetadata = fontTextureAtlasMetadata.OriginalFontMetadata;

    //
    // Reset quad vertices
    //

    context.TextQuadVertexBuffer.clear();

    //
    // Rebuild quad vertices
    //

    // Hardcoded pixel offsets of readings in physics probe panel,
    // relative to bottom-right corner
    vec2f constexpr PhysicsProbePanelSpeedBottomRight(137.0f, 12.0f);
    vec2f constexpr PhysicsProbePanelTemperatureBottomRight(295.0f, 12.0f);

    for (auto const & textLine : context.TextLines)
    {
        //
        // Calculate line position in NDC coordinates
        //

        vec2f linePositionNdc( // Top-left of quads; start with line's offset
            textLine.ScreenOffset.x * static_cast<float>(fontMetadata.GetCellScreenWidth()) * mScreenToNdcX,
            -textLine.ScreenOffset.y * static_cast<float>(fontMetadata.GetCellScreenHeight()) * mScreenToNdcY);

        switch (textLine.Anchor)
        {
            case NotificationAnchorPositionType::BottomLeft:
            {
                linePositionNdc += vec2f(
                    -1.f + MarginScreen * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(fontMetadata.GetCellScreenHeight())) * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::BottomRight:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    1.f - (MarginScreen + static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(lineExtent.Height)) * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::TopLeft:
            {
                linePositionNdc += vec2f(
                    -1.f + MarginScreen * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::TopRight:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    1.f - (MarginScreen + static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::PhysicsProbeReadingSpeed:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    -1.f + (PhysicsProbePanelSpeedBottomRight.x - static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
                    -1.f + PhysicsProbePanelSpeedBottomRight.y * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::PhysicsProbeReadingTemperature:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    -1.f + (PhysicsProbePanelTemperatureBottomRight.x - static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
                    -1.f + PhysicsProbePanelTemperatureBottomRight.y * mScreenToNdcY);

                break;
            }
        }

        //
        // Emit quads for this line
        //

        float const alpha = textLine.Alpha;
        auto & vertices = context.TextQuadVertexBuffer;

        for (char _ch : textLine.Text)
        {
            unsigned char const ch = static_cast<unsigned char>(_ch);

            float const glyphWidthNdc = static_cast<float>(fontTextureAtlasMetadata.OriginalFontMetadata.GetGlyphScreenWidth(ch)) * mScreenToNdcX;
            float const glyphHeightNdc = static_cast<float>(fontTextureAtlasMetadata.OriginalFontMetadata.GetGlyphScreenHeight(ch)) * mScreenToNdcY;

            float const textureULeft = fontTextureAtlasMetadata.GlyphTextureAtlasBottomLefts[ch].x;
            float const textureURight = fontTextureAtlasMetadata.GlyphTextureAtlasTopRights[ch].x;
            float const textureVBottom = fontTextureAtlasMetadata.GlyphTextureAtlasBottomLefts[ch].y;
            float const textureVTop = fontTextureAtlasMetadata.GlyphTextureAtlasTopRights[ch].y;

            // Top-left
            vertices.emplace_back(
                linePositionNdc.x,
                linePositionNdc.y + glyphHeightNdc,
                textureULeft,
                textureVTop,
                alpha);

            // Bottom-left
            vertices.emplace_back(
                linePositionNdc.x,
                linePositionNdc.y,
                textureULeft,
                textureVBottom,
                alpha);

            // Top-right
            vertices.emplace_back(
                linePositionNdc.x + glyphWidthNdc,
                linePositionNdc.y + glyphHeightNdc,
                textureURight,
                textureVTop,
                alpha);

            // Bottom-left
            vertices.emplace_back(
                linePositionNdc.x,
                linePositionNdc.y,
                textureULeft,
                textureVBottom,
                alpha);

            // Top-right
            vertices.emplace_back(
                linePositionNdc.x + glyphWidthNdc,
                linePositionNdc.y + glyphHeightNdc,
                textureURight,
                textureVTop,
                alpha);

            // Bottom-right
            vertices.emplace_back(
                linePositionNdc.x + glyphWidthNdc,
                linePositionNdc.y,
                textureURight,
                textureVBottom,
                alpha);

            linePositionNdc.x += glyphWidthNdc;
        }
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

        // Top-right
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

        // Top-right
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