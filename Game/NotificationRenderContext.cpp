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
    GlobalRenderContext & globalRenderContext)
    : mGlobalRenderContext(globalRenderContext)
    , mShaderManager(shaderManager)
    , mScreenToNdcX(0.0f) // Will be recalculated
    , mScreenToNdcY(0.0f) // Will be recalculated
    // Textures
    , mGenericLinearTextureAtlasMetadata(globalRenderContext.GetGenericLinearTextureAtlasMetadata())
    , mGenericMipMappedTextureAtlasMetadata(globalRenderContext.GetGenericMipMappedTextureAtlasMetadata())
    // Text
    , mFontTextureAtlasMetadata()
    , mTextNotificationTypeContexts()
    , mTextVAO()
    , mCurrentTextQuadVertexBufferSize(0)
    , mAllocatedTextQuadVertexBufferSize(0)
    , mTextVBO()
    , mFontAtlasTextureHandle()
    // Texture notifications
    , mTextureNotifications()
    , mIsTextureNotificationDataDirty(false) // We're ok with initial state (empty)
    , mTextureNotificationVAO()
    , mTextureNotificationVertexBuffer()
    , mTextureNotificationVBO()
    // Physics probe panel
    , mPhysicsProbePanel()
    , mIsPhysicsProbeDataDirty(false) // We're ok with initial state (empty)
    , mPhysicsProbePanelVAO()
    , mPhysicsProbePanelVertexBuffer()
    , mPhysicsProbePanelVBO()
    // Tool notifications
    , mHeatBlasterFlameVAO()
    , mHeatBlasterFlameVBO()
    , mHeatBlasterFlameShaderToRender()
    , mFireExtinguisherSprayVAO()
    , mFireExtinguisherSprayVBO()
    , mFireExtinguisherSprayShaderToRender()
    , mWindSphereVAO()
    , mWindSphereVBO()
    , mLaserCannonVAO()
    , mLaserCannonVBO()
    , mLaserRayVAO()
    , mLaserRayVBO()
    , mMultiNotificationVAO()
    , mMultiNotificationVBO()
    , mRectSelectionVAO()
    , mRectSelectionVBO()
    , mInteractiveToolDashedLineVAO()
    , mInteractiveToolDashedLineVBO()
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
            static_cast<float>(fonts[f].Texture.Size.width),
            static_cast<float>(fonts[f].Texture.Size.height),
            false,
            ImageCoordinates(0, 0),
            vec2f::zero(),
            TextureFrameId<FontTextureGroups>(
                FontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)),
            std::to_string(f),
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fontTextureAtlas.AtlasData.Size.width, fontTextureAtlas.AtlasData.Size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fontTextureAtlas.AtlasData.Data.get());
    CheckOpenGLError();

    glBindTexture(GL_TEXTURE_2D, 0);

    //
    // Initialize text notifications
    //

    {

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
        static_assert(sizeof(TextQuadVertex) == (4 + 1) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mTextVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Text1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Text1), 4, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Text2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Text2), 1, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void*)(4 * sizeof(float)));
        CheckOpenGLError();

        //
        // Associate element VBO
        //

        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO. So we won't associate the element VBO here, but rather before each drawing call.
        ////mGlobalRenderContext.GetElementIndices().Bind()

        glBindVertexArray(0);
    }

    // Initialize font texture atlas metadata
    for (size_t f = 0; f < fonts.size(); ++f)
    {
        auto const & fontTextureFrameMetadata = fontTextureAtlas.Metadata.GetFrameMetadata(
            TextureFrameId<FontTextureGroups>(
                FontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)));

        // Dimensions of a cell of this font, in the atlas' texture space coordinates
        float const fontCellWidthAtlasTextureSpace = static_cast<float>(fonts[f].Metadata.GetCellScreenWidth()) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().width);
        float const fontCellHeightAtlasTextureSpace = static_cast<float>(fonts[f].Metadata.GetCellScreenHeight()) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().height);

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
                + static_cast<float>(fonts[f].Metadata.GetGlyphScreenWidth(c) - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().width);

            // Texture-space top y
            // Note: font texture is flipped vertically (top of character is at lower V coordinates)
            int const glyphTextureRow = (c - FontMetadata::BaseCharacter) / fonts[f].Metadata.GetGlyphsPerTextureRow();
            float const glyphTopAtlasTextureSpace =
                fontTextureFrameMetadata.TextureCoordinatesBottomLeft.y // Includes dead-center dx already
                + static_cast<float>(glyphTextureRow) * fontCellHeightAtlasTextureSpace;

            float const glyphBottomAtlasTextureSpace =
                glyphTopAtlasTextureSpace
                + static_cast<float>(fonts[f].Metadata.GetGlyphScreenHeight(c) - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().height);

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
        mTextNotificationTypeContexts[static_cast<size_t>(TextNotificationType::StatusText)] =
            TextNotificationTypeContext(&(mFontTextureAtlasMetadata[static_cast<size_t>(FontType::Font0)]));

        // Notification text
        mTextNotificationTypeContexts[static_cast<size_t>(TextNotificationType::NotificationText)] =
            TextNotificationTypeContext(&(mFontTextureAtlasMetadata[static_cast<size_t>(FontType::Font1)]));

        // Physics probe reading
        mTextNotificationTypeContexts[static_cast<size_t>(TextNotificationType::PhysicsProbeReading)] =
            TextNotificationTypeContext(&(mFontTextureAtlasMetadata[static_cast<size_t>(FontType::Font2)]));
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
        static_assert(sizeof(TextureNotificationVertex) == (4 + 1) * sizeof(float));
        glBindVertexArray(*mTextureNotificationVAO);
        glBindBuffer(GL_ARRAY_BUFFER, *mTextureNotificationVBO);
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
        static_assert(sizeof(PhysicsProbePanelVertex) == 7 * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mPhysicsProbePanelVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel1), 4, GL_FLOAT, GL_FALSE, sizeof(PhysicsProbePanelVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::PhysicsProbePanel2), 3, GL_FLOAT, GL_FALSE, sizeof(PhysicsProbePanelVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set texture parameters
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
        static_assert(sizeof(HeatBlasterFlameVertex) == 4 * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mHeatBlasterFlameVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame), 4, GL_FLOAT, GL_FALSE, sizeof(HeatBlasterFlameVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set texture parameters
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
        static_assert(sizeof(FireExtinguisherSprayVertex) == 4 * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mFireExtinguisherSprayVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray), 4, GL_FLOAT, GL_FALSE, sizeof(FireExtinguisherSprayVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set texture parameters
        mShaderManager.ActivateProgram<ProgramType::FireExtinguisherSpray>();
        mShaderManager.SetTextureParameters<ProgramType::FireExtinguisherSpray>();
    }

    //
    // Initialize Wind Sphere
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mWindSphereVAO = tmpGLuint;

        glBindVertexArray(*mWindSphereVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mWindSphereVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(WindSphereVertex) == (4 + 4) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mWindSphereVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::WindSphere1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::WindSphere1), 4, GL_FLOAT, GL_FALSE, sizeof(WindSphereVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::WindSphere2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::WindSphere2), 4, GL_FLOAT, GL_FALSE, sizeof(WindSphereVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set texture parameters
        mShaderManager.ActivateProgram<ProgramType::WindSphere>();
        mShaderManager.SetTextureParameters<ProgramType::WindSphere>();
    }

    //
    // Initialize Laser Cannon
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mLaserCannonVAO = tmpGLuint;

        glBindVertexArray(*mLaserCannonVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mLaserCannonVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(LaserCannonVertex) == (4 + 3) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mLaserCannonVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTextureNdc1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTextureNdc1), 4, GL_FLOAT, GL_FALSE, sizeof(LaserCannonVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTextureNdc2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTextureNdc2), 3, GL_FLOAT, GL_FALSE, sizeof(LaserCannonVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Laser Ray
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mLaserRayVAO = tmpGLuint;

        glBindVertexArray(*mLaserRayVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mLaserRayVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(LaserRayVertex) == (4 + 1) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mLaserRayVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::LaserRay1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::LaserRay1), 4, GL_FLOAT, GL_FALSE, sizeof(LaserRayVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::LaserRay2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::LaserRay2), 1, GL_FLOAT, GL_FALSE, sizeof(LaserRayVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set texture parameters
        mShaderManager.ActivateProgram<ProgramType::LaserRay>();
        mShaderManager.SetTextureParameters<ProgramType::LaserRay>();
    }

    //
    // Initialize Multi-Notification
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mMultiNotificationVAO = tmpGLuint;

        glBindVertexArray(*mMultiNotificationVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mMultiNotificationVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(MultiNotificationVertex) == (1 + 6) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mMultiNotificationVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::MultiNotification1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::MultiNotification1), 4, GL_FLOAT, GL_FALSE, sizeof(MultiNotificationVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::MultiNotification2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::MultiNotification2), 3, GL_FLOAT, GL_FALSE, sizeof(MultiNotificationVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set texture parameters
        mShaderManager.ActivateProgram<ProgramType::MultiNotification>();
        mShaderManager.SetTextureParameters<ProgramType::MultiNotification>();

        // Prepare buffer
        mMultiNotificationVertexBuffer.reserve(6 * 4); // Arbitrary
    }

    //
    // Initialize Rect Selection Ray
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mRectSelectionVAO = tmpGLuint;

        glBindVertexArray(*mRectSelectionVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mRectSelectionVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(RectSelectionVertex) == (2 + 2 + 2 + 2 + 3 + 1) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mRectSelectionVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::RectSelection1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::RectSelection1), 4, GL_FLOAT, GL_FALSE, sizeof(RectSelectionVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::RectSelection2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::RectSelection2), 4, GL_FLOAT, GL_FALSE, sizeof(RectSelectionVertex), (void *)(4 * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::RectSelection3));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::RectSelection3), 4, GL_FLOAT, GL_FALSE, sizeof(RectSelectionVertex), (void *)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Interactive Tool Line Guide
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mInteractiveToolDashedLineVAO = tmpGLuint;

        glBindVertexArray(*mInteractiveToolDashedLineVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mInteractiveToolDashedLineVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(InteractiveToolDashedLineVertex) == (2 + 1) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mInteractiveToolDashedLineVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::InteractiveToolDashedLine1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::InteractiveToolDashedLine1), (2 + 1), GL_FLOAT, GL_FALSE, sizeof(InteractiveToolDashedLineVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void NotificationRenderContext::UploadStart()
{
    // Reset HeatBlaster flame, it's uploaded as needed
    mHeatBlasterFlameShaderToRender.reset();

    // Reset fire extinguisher spray, it's uploaded as needed
    mFireExtinguisherSprayShaderToRender.reset();

    // Reset wind sphere, it's uploaded as needed
    mWindSphereVertexBuffer.clear();

    // Reset laser cannon, it's uploaded as needed
    mLaserCannonVertexBuffer.clear();

    // Reset laser ray, it's uploaded as needed
    mLaserRayVertexBuffer.clear();

    // Reset multi-notifications, they are uploaded as needed
    mMultiNotificationVertexBuffer.clear();

    // Reset rect selection, it's uploaded as needed
    mRectSelectionVertexBuffer.clear();

    // Reset InteractiveToolDashedLines, they are uploaded as needed
    mInteractiveToolDashedLineVertexBuffer.clear();
}

void NotificationRenderContext::UploadLaserCannon(
    DisplayLogicalCoordinates const & screenCenter,
    std::optional<float> strength,
    ViewModel const & viewModel)
{
    //
    // Calculations are all in screen (logical display) coordinates
    //

    float const width = static_cast<float>(viewModel.GetCanvasLogicalSize().width);
    float const height = static_cast<float>(viewModel.GetCanvasLogicalSize().height);

    vec2f const screenCenterF = screenCenter.ToFloat().clamp(0.0f, width, 0.0f, height);

    std::array<vec2f, 4> const screenCorners{
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, height),
        vec2f(width, 0.0f),
        vec2f(width, height)
    };

    auto const & frameMetadata = mGenericMipMappedTextureAtlasMetadata.GetFrameMetadata(
        TextureFrameId<GenericMipMappedTextureGroups>(GenericMipMappedTextureGroups::LaserCannon, 0));

    float const ambientLightSensitivity = frameMetadata.FrameMetadata.HasOwnAmbientLight ? 0.0f : 1.0f;

    float const screenCannonLength = static_cast<float>(frameMetadata.FrameMetadata.Size.height);
    float const screenCannonWidth = static_cast<float>(frameMetadata.FrameMetadata.Size.width);

    float const screenRayWidth = 17.0f; // Based on cannon PNG
    float const screenRayWidthEnd = screenRayWidth * std::min(viewModel.GetZoom(), 1.0f); // Taper ray towards center, depending on zoom: the further (smaller), the more tapered

    // Process all corners
    for (vec2f const & screenCorner : screenCorners)
    {
        vec2f const screenRay = screenCenterF - screenCorner;
        float const screenRayLength = screenRay.length();
        // Skip cannon if too short
        if (screenRayLength > 1.0f)
        {
            vec2f rayDir = screenRay.normalise(screenRayLength);
            vec2f const rayPerpDir = rayDir.to_perpendicular();

            //
            // Create cannon vertices
            //

            // Cannon origin: H=mid, V=bottom, calculated considering retreat when there is not enough room
            vec2f const screenOrigin = screenCorner - rayDir * std::max(screenCannonLength - screenRayLength, 0.0f);

            vec2f const ndcCannonBottomLeft = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenOrigin + rayPerpDir * screenCannonWidth / 2.0f));
            vec2f const ndcCannonBottomRight = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenOrigin - rayPerpDir * screenCannonWidth / 2.0f));
            vec2f const ndcCannonTopLeft = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenOrigin + rayDir * screenCannonLength + rayPerpDir * screenCannonWidth / 2.0f));
            vec2f const ndcCannonTopRight = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenOrigin + rayDir * screenCannonLength - rayPerpDir * screenCannonWidth / 2.0f));

            // Bottom-left
            mLaserCannonVertexBuffer.emplace_back(
                ndcCannonBottomLeft,
                frameMetadata.TextureCoordinatesBottomLeft,
                1.0f, // PlaneID
                1.0f, // Alpha
                ambientLightSensitivity);

            // Top-left
            mLaserCannonVertexBuffer.emplace_back(
                ndcCannonTopLeft,
                vec2f(frameMetadata.TextureCoordinatesBottomLeft.x, frameMetadata.TextureCoordinatesTopRight.y),
                1.0f, // PlaneID
                1.0f, // Alpha
                ambientLightSensitivity);

            // Bottom-right
            mLaserCannonVertexBuffer.emplace_back(
                ndcCannonBottomRight,
                vec2f(frameMetadata.TextureCoordinatesTopRight.x, frameMetadata.TextureCoordinatesBottomLeft.y),
                1.0f, // PlaneID
                1.0f, // Alpha
                ambientLightSensitivity);

            // Top-left
            mLaserCannonVertexBuffer.emplace_back(
                ndcCannonTopLeft,
                vec2f(frameMetadata.TextureCoordinatesBottomLeft.x, frameMetadata.TextureCoordinatesTopRight.y),
                1.0f, // PlaneID
                1.0f, // Alpha
                ambientLightSensitivity);

            // Bottom-right
            mLaserCannonVertexBuffer.emplace_back(
                ndcCannonBottomRight,
                vec2f(frameMetadata.TextureCoordinatesTopRight.x, frameMetadata.TextureCoordinatesBottomLeft.y),
                1.0f, // PlaneID
                1.0f, // Alpha
                ambientLightSensitivity);

            // Top-right
            mLaserCannonVertexBuffer.emplace_back(
                ndcCannonTopRight,
                frameMetadata.TextureCoordinatesTopRight,
                1.0f, // PlaneID
                1.0f, // Alpha
                ambientLightSensitivity);

            if (strength)
            {
                //
                // Create ray vertices
                //

                vec2f const ndcRayBottomLeft = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenOrigin + rayPerpDir * screenRayWidth / 2.0f));
                vec2f const ndcRayBottomRight = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenOrigin - rayPerpDir * screenRayWidth / 2.0f));
                vec2f const ndcRayTopLeft = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenCenterF + rayPerpDir * screenRayWidthEnd / 2.0f));
                vec2f const ndcRayTopRight = viewModel.ScreenToNdc(DisplayLogicalCoordinates::FromFloatRound(screenCenterF - rayPerpDir * screenRayWidthEnd / 2.0f));

                // Ray space: tip Y is +1.0, bottom Y follows ray length so that shorter rays are not denser than longer rays
                float const raySpaceYBottom = 1.0f - (ndcRayTopLeft - ndcRayBottomLeft).length() / 1.4142f;

                // Bottom-left
                mLaserRayVertexBuffer.emplace_back(
                    ndcRayBottomLeft,
                    vec2f(-1.0f, raySpaceYBottom),
                    *strength);

                // Top-left
                mLaserRayVertexBuffer.emplace_back(
                    ndcRayTopLeft,
                    vec2f(-1.0f, 1.0f),
                    *strength);

                // Bottom-right
                mLaserRayVertexBuffer.emplace_back(
                    ndcRayBottomRight,
                    vec2f(1.0f, raySpaceYBottom),
                    *strength);

                // Top-left
                mLaserRayVertexBuffer.emplace_back(
                    ndcRayTopLeft,
                    vec2f(-1.0f, 1.0f),
                    *strength);

                // Bottom-right
                mLaserRayVertexBuffer.emplace_back(
                    ndcRayBottomRight,
                    vec2f(1.0f, raySpaceYBottom),
                    *strength);

                // Top-right
                mLaserRayVertexBuffer.emplace_back(
                    ndcRayTopRight,
                    vec2f(1.0f, 1.0f),
                    *strength);
            }
        }
    }
}

void NotificationRenderContext::UploadInteractiveToolDashedLine(
    DisplayLogicalCoordinates const & screenStart,
    DisplayLogicalCoordinates const & screenEnd,
    ViewModel const & viewModel)
{
    //
    // Create line vertices
    //

    vec2f const ndcStart = viewModel.ScreenToNdc(screenStart);
    vec2f const ndcEnd = viewModel.ScreenToNdc(screenEnd);

    float pixelLength = (screenEnd.ToFloat() - screenStart.ToFloat()).length();

    // Normalize length so it's a multiple of the period + 1/2 period
    float constexpr DashPeriod = 16.0f; // 8 + 8
    float const leftover = std::fmod(pixelLength + DashPeriod / 2.0f, DashPeriod);
    pixelLength += (DashPeriod - leftover);

    mInteractiveToolDashedLineVertexBuffer.emplace_back(
        ndcStart,
        0.0f);

    mInteractiveToolDashedLineVertexBuffer.emplace_back(
        ndcEnd,
        pixelLength);
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

    if (renderParameters.IsDisplayUnitsSystemDirty)
    {
        ApplyDisplayUnitsSystemChanges(renderParameters);
    }
}

void NotificationRenderContext::RenderPrepare()
{
    RenderPrepareTextNotifications();

    RenderPrepareTextureNotifications();

    RenderPreparePhysicsProbePanel();

    RenderPrepareHeatBlasterFlame();

    RenderPrepareFireExtinguisherSpray();

    RenderPrepareWindSphere();

    RenderPrepareLaserCannon();

    RenderPrepareLaserRay();

    RenderPrepareMultiNotification();

    RenderPrepareRectSelection();

    RenderPrepareInteractiveToolDashedLines();
}

void NotificationRenderContext::RenderDraw()
{
    //
    // Set gross noise in the noise texture unit, as all our shaders require that one
    //

    mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture>();
    glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Gross));

    //
    // Draw
    //
    // Note the Z-order here!
    //

    RenderDrawLaserRay();
    RenderDrawLaserCannon();

    RenderDrawPhysicsProbePanel();

    RenderDrawTextNotifications();

    RenderDrawTextureNotifications();

    RenderDrawHeatBlasterFlame();

    RenderDrawFireExtinguisherSpray();

    RenderDrawWindSphere();

    RenderDrawMultiNotification();

    RenderDrawRectSelection();

    RenderDrawInteractiveToolDashedLines();
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

    mShaderManager.ActivateProgram<ProgramType::WindSphere>();
    mShaderManager.SetProgramParameter<ProgramType::WindSphere, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::MultiNotification>();
    mShaderManager.SetProgramParameter<ProgramType::MultiNotification, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::RectSelection>();
    mShaderManager.SetProgramParameter<ProgramType::RectSelection, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);
}

void NotificationRenderContext::ApplyCanvasSizeChanges(RenderParameters const & renderParameters)
{
    auto const & view = renderParameters.View;

    // Recalculate screen -> NDC conversion factors
    mScreenToNdcX = 2.0f / static_cast<float>(view.GetCanvasPhysicalSize().width);
    mScreenToNdcY = 2.0f / static_cast<float>(view.GetCanvasPhysicalSize().height);

    // Make sure we re-calculate (and re-upload) all text vertices
    // at the next iteration
    for (auto & tntc : mTextNotificationTypeContexts)
    {
        tntc.AreTextLinesDirty = true;
    }

    // Make sure we re-calculate (and re-upload) all texture notification vertices
    // at the next iteration
    mIsTextureNotificationDataDirty = true;

    // Make sure we re-calculate (and re-upload) the physics probe panel
    // at the next iteration
    mIsPhysicsProbeDataDirty = true;
}

void NotificationRenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters)
{
    // Set parameter in all programs

    float const lighteningStrength = Step(0.5f, 1.0f - renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::Text>();
    mShaderManager.SetProgramParameter<ProgramType::Text, ProgramParameterType::TextLighteningStrength>(
        lighteningStrength);

    mShaderManager.ActivateProgram<ProgramType::TextureNotifications>();
    mShaderManager.SetProgramParameter<ProgramType::TextureNotifications, ProgramParameterType::TextureLighteningStrength>(
        lighteningStrength);
}

void NotificationRenderContext::ApplyDisplayUnitsSystemChanges(RenderParameters const & renderParameters)
{
    TextureFrameIndex frameIndex{ 0 };
    switch (renderParameters.DisplayUnitsSystem)
    {
        case UnitsSystem::SI_Celsius:
        {
            // Frame 1
            frameIndex = 1;
            break;
        }

        case UnitsSystem::SI_Kelvin:
        {
            // Frame 0
            frameIndex = 0;
            break;
        }

        case UnitsSystem::USCS:
        {
            // Frame 2
            frameIndex = 2;
            break;
        }
    }

    auto const & frameMetadata = mGenericLinearTextureAtlasMetadata.GetFrameMetadata(TextureFrameId<GenericLinearTextureGroups>(GenericLinearTextureGroups::PhysicsProbePanel, frameIndex));

    // Set texture offset in program
    mShaderManager.ActivateProgram<ProgramType::PhysicsProbePanel>();
    mShaderManager.SetProgramParameter<ProgramType::PhysicsProbePanel, ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates>(frameMetadata.TextureCoordinatesBottomLeft);
}

void NotificationRenderContext::RenderPrepareTextNotifications()
{
    //
    // Check whether we need to re-generate - and thus re-upload - quad vertex buffers
    //

    bool doNeedToUploadQuadVertexBuffers = false;
    size_t totalTextQuadVertexBufferSize = 0;

    for (auto & textNotificationTypeContext : mTextNotificationTypeContexts)
    {
        if (textNotificationTypeContext.AreTextLinesDirty)
        {
            // Re-generate quad vertices for this notification type
            GenerateTextVertices(textNotificationTypeContext);

            textNotificationTypeContext.AreTextLinesDirty = false;

            // We need to re-upload the vertex buffers
            doNeedToUploadQuadVertexBuffers = true;
        }

        totalTextQuadVertexBufferSize += textNotificationTypeContext.TextQuadVertexBuffer.size();
    }

    if (doNeedToUploadQuadVertexBuffers)
    {
        //
        // Re-upload whole buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mTextVBO);

        // Update total buffer size
        mCurrentTextQuadVertexBufferSize = totalTextQuadVertexBufferSize;

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

        //
        // Ensure element indices cover whole text
        //

        assert((mCurrentTextQuadVertexBufferSize % 4) == 0);
        mGlobalRenderContext.GetElementIndices().EnsureSize(mCurrentTextQuadVertexBufferSize / 4);
    }
}

void NotificationRenderContext::RenderDrawTextNotifications()
{
    if (mCurrentTextQuadVertexBufferSize > 0)
    {
        glBindVertexArray(*mTextVAO);

        // Intel bug: cannot associate with VAO
        mGlobalRenderContext.GetElementIndices().Bind();

        // Activate texture unit
        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

        // Bind font atlas texture
        glBindTexture(GL_TEXTURE_2D, *mFontAtlasTextureHandle);
        CheckOpenGLError();

        // Activate program
        mShaderManager.ActivateProgram<ProgramType::Text>();

        // Draw vertices
        assert(0 == (mCurrentTextQuadVertexBufferSize % 4));
        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(mCurrentTextQuadVertexBufferSize / 4 * 6),
            GL_UNSIGNED_INT,
            (GLvoid *)0);
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
    if (mIsPhysicsProbeDataDirty)
    {
        //
        // Recalculate NDC dimensions of physics probe panel
        //

        auto const & atlasFrame = mGenericLinearTextureAtlasMetadata.GetFrameMetadata(TextureFrameId<GenericLinearTextureGroups>(GenericLinearTextureGroups::PhysicsProbePanel, 0));
        vec2f const physicsProbePanelNdcDimensions = vec2f(
            static_cast<float>(atlasFrame.FrameMetadata.Size.width) * mScreenToNdcX,
            static_cast<float>(atlasFrame.FrameMetadata.Size.height) * mScreenToNdcY);

        // Set parameters
        mShaderManager.ActivateProgram<ProgramType::PhysicsProbePanel>();
        mShaderManager.SetProgramParameter<ProgramType::PhysicsProbePanel, ProgramParameterType::WidthNdc>(
            physicsProbePanelNdcDimensions.x);

        //
        // Generate vertices
        //

        mPhysicsProbePanelVertexBuffer.clear();

        if (mPhysicsProbePanel)
        {
            //
            // Generate quad
            //

            // First 1/3rd of open: grow vertically
            // Last 2/3rds of open: grow horizontally

            float constexpr VerticalOpenFraction = 0.3333f;

            float const verticalOpen = (mPhysicsProbePanel->Open < VerticalOpenFraction)
                ? mPhysicsProbePanel->Open / VerticalOpenFraction
                : 1.0f;

            float const MinHorizontalOpen = 0.0125f;

            float const horizontalOpen = (mPhysicsProbePanel->Open < VerticalOpenFraction)
                ? MinHorizontalOpen
                : MinHorizontalOpen + (1.0f - MinHorizontalOpen) * (mPhysicsProbePanel->Open - VerticalOpenFraction) / (1.0f - VerticalOpenFraction);

            float const midYNdc = -1.f + physicsProbePanelNdcDimensions.y / 2.0f;

            vec2f const quadTopLeft = vec2f(
                -1.0f,
                midYNdc + verticalOpen * (physicsProbePanelNdcDimensions.y / 2.0f));

            vec2f const quadBottomRight = vec2f(
                -1.0f + physicsProbePanelNdcDimensions.x,
                midYNdc - verticalOpen * (physicsProbePanelNdcDimensions.y / 2.0f));

            vec2f const xLimits = vec2f(
                quadTopLeft.x + physicsProbePanelNdcDimensions.x / 2.0f * (1.0f - horizontalOpen),
                quadBottomRight.x - physicsProbePanelNdcDimensions.x / 2.0f * (1.0f - horizontalOpen));

            float opening = mPhysicsProbePanel->IsOpening ? 1.0f : 0.0f;

            // Get texture NDC dimensions (assuming all panels have equal dimensions)
            float const textureWidth = atlasFrame.TextureCoordinatesTopRight.x - atlasFrame.TextureCoordinatesBottomLeft.x;
            float const textureHeight = atlasFrame.TextureCoordinatesTopRight.y - atlasFrame.TextureCoordinatesBottomLeft.y;

            // Triangle 1

            // Top-left
            mPhysicsProbePanelVertexBuffer.emplace_back(
                quadTopLeft,
                vec2f(0.0f, textureHeight),
                xLimits,
                opening);

            // Top-right
            mPhysicsProbePanelVertexBuffer.emplace_back(
                vec2f(quadBottomRight.x, quadTopLeft.y),
                vec2f(textureWidth, textureHeight),
                xLimits,
                opening);

            // Bottom-left
            mPhysicsProbePanelVertexBuffer.emplace_back(
                vec2f(quadTopLeft.x, quadBottomRight.y),
                vec2f(0.0f, 0.0f),
                xLimits,
                opening);

            // Triangle 2

            // Top-right
            mPhysicsProbePanelVertexBuffer.emplace_back(
                vec2f(quadBottomRight.x, quadTopLeft.y),
                vec2f(textureWidth, textureHeight),
                xLimits,
                opening);

            // Bottom-left
            mPhysicsProbePanelVertexBuffer.emplace_back(
                vec2f(quadTopLeft.x, quadBottomRight.y),
                vec2f(0.0f, 0.0f),
                xLimits,
                opening);

            // Bottom-right
            mPhysicsProbePanelVertexBuffer.emplace_back(
                quadBottomRight,
                vec2f(textureWidth, 0.0f),
                xLimits,
                opening);

            //
            // Upload buffer
            //

            glBindBuffer(GL_ARRAY_BUFFER, *mPhysicsProbePanelVBO);

            glBufferData(GL_ARRAY_BUFFER,
                sizeof(PhysicsProbePanelVertex) * mPhysicsProbePanelVertexBuffer.size(),
                mPhysicsProbePanelVertexBuffer.data(),
                GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        mIsPhysicsProbeDataDirty = false;
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

void NotificationRenderContext::RenderPrepareWindSphere()
{
    if (!mWindSphereVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mWindSphereVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(WindSphereVertex) * mWindSphereVertexBuffer.size(),
            mWindSphereVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Set time parameter
        mShaderManager.ActivateProgram<ProgramType::WindSphere>();
        mShaderManager.SetProgramParameter<ProgramParameterType::Time>(
            ProgramType::WindSphere,
            GameWallClock::GetInstance().NowAsFloat());
    }
}

void NotificationRenderContext::RenderDrawWindSphere()
{
    if (!mWindSphereVertexBuffer.empty())
    {
        glBindVertexArray(*mWindSphereVAO);

        mShaderManager.ActivateProgram<ProgramType::WindSphere>();

        // Draw
        assert((mWindSphereVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mWindSphereVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareLaserCannon()
{
    if (!mLaserCannonVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mLaserCannonVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(LaserCannonVertex) * mLaserCannonVertexBuffer.size(),
            mLaserCannonVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawLaserCannon()
{
    if (!mLaserCannonVertexBuffer.empty())
    {
        glBindVertexArray(*mLaserCannonVAO);

        mShaderManager.ActivateProgram<ProgramType::GenericMipMappedTexturesNdc>();

        // Draw
        assert((mLaserCannonVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mLaserCannonVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareLaserRay()
{
    if (!mLaserRayVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mLaserRayVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(LaserRayVertex) * mLaserRayVertexBuffer.size(),
            mLaserRayVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Set time parameter
        mShaderManager.ActivateProgram<ProgramType::LaserRay>();
        mShaderManager.SetProgramParameter<ProgramParameterType::Time>(
            ProgramType::LaserRay,
            GameWallClock::GetInstance().NowAsFloat());
    }
}

void NotificationRenderContext::RenderDrawLaserRay()
{
    if (!mLaserRayVertexBuffer.empty())
    {
        glBindVertexArray(*mLaserRayVAO);

        mShaderManager.ActivateProgram<ProgramType::LaserRay>();

        // Draw
        assert((mLaserRayVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mLaserRayVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareMultiNotification()
{
    if (!mMultiNotificationVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mMultiNotificationVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(MultiNotificationVertex) * mMultiNotificationVertexBuffer.size(),
            mMultiNotificationVertexBuffer.data(),
            GL_STREAM_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Set time parameter
        mShaderManager.ActivateProgram<ProgramType::MultiNotification>();
        mShaderManager.SetProgramParameter<ProgramParameterType::Time>(
            ProgramType::MultiNotification,
            GameWallClock::GetInstance().NowAsFloat());
    }
}

void NotificationRenderContext::RenderDrawMultiNotification()
{
    if (!mMultiNotificationVertexBuffer.empty())
    {
        glBindVertexArray(*mMultiNotificationVAO);

        mShaderManager.ActivateProgram<ProgramType::MultiNotification>();

        bool doResetBlending = false;
        if (mMultiNotificationVertexBuffer[0].vertexKind == static_cast<float>(MultiNotificationVertex::VertexKindType::BlastToolHalo)
            || mMultiNotificationVertexBuffer[0].vertexKind == static_cast<float>(MultiNotificationVertex::VertexKindType::PressureInjectionHalo))
        {
            // Setup custom blending
            glBlendFunc(GL_SRC_COLOR, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);

            doResetBlending = true;
        }

        // Draw
        assert((mMultiNotificationVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mMultiNotificationVertexBuffer.size()));

        if (doResetBlending)
        {
            // Reset blending
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendEquation(GL_FUNC_ADD);
        }

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareRectSelection()
{
    if (!mRectSelectionVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mRectSelectionVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(RectSelectionVertex) * mRectSelectionVertexBuffer.size(),
            mRectSelectionVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawRectSelection()
{
    if (!mRectSelectionVertexBuffer.empty())
    {
        glBindVertexArray(*mRectSelectionVAO);

        mShaderManager.ActivateProgram<ProgramType::RectSelection>();

        // Draw
        assert((mRectSelectionVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mRectSelectionVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPrepareInteractiveToolDashedLines()
{
    if (!mInteractiveToolDashedLineVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mInteractiveToolDashedLineVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(InteractiveToolDashedLineVertex) * mInteractiveToolDashedLineVertexBuffer.size(),
            mInteractiveToolDashedLineVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawInteractiveToolDashedLines()
{
    if (!mInteractiveToolDashedLineVertexBuffer.empty())
    {
        // Bind VAO
        glBindVertexArray(*mInteractiveToolDashedLineVAO);

        // Activate program
        mShaderManager.ActivateProgram<ProgramType::InteractiveToolDashedLines>();

        // Set line width
        glLineWidth(2.0f);

        // Draw
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mInteractiveToolDashedLineVertexBuffer.size()));
        CheckOpenGLError();
    }
}

void NotificationRenderContext::GenerateTextVertices(TextNotificationTypeContext & context) const
{
    FontTextureAtlasMetadata const & fontTextureAtlasMetadata = *(context.NotificationFontTextureAtlasMetadata);
    FontMetadata const & fontMetadata = fontTextureAtlasMetadata.OriginalFontMetadata;

    //
    // Reset quad vertices
    //

    context.TextQuadVertexBuffer.clear();

    //
    // Rebuild quad vertices
    //

    // Hardcoded pixel offsets of readings in physics probe panel,
    // giving position of text's bottom-right corner
    float constexpr PhysicsProbePanelTextBottomY = 10.0f;
    vec2f constexpr PhysicsProbePanelSpeedBottomRight(101.0f, PhysicsProbePanelTextBottomY);
    vec2f constexpr PhysicsProbePanelTemperatureBottomRight(235.0f, PhysicsProbePanelTextBottomY);
    vec2f constexpr PhysicsProbePanelDepthBottomRight(371.0f, PhysicsProbePanelTextBottomY);
    vec2f constexpr PhysicsProbePanelPressureBottomRight(506.0f, PhysicsProbePanelTextBottomY);

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
                    1.f - (MarginScreen + static_cast<float>(lineExtent.width)) * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(lineExtent.height)) * mScreenToNdcY);

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
                    1.f - (MarginScreen + static_cast<float>(lineExtent.width)) * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::PhysicsProbeReadingDepth:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    -1.f + (PhysicsProbePanelDepthBottomRight.x - static_cast<float>(lineExtent.width)) * mScreenToNdcX,
                    -1.f + PhysicsProbePanelDepthBottomRight.y * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::PhysicsProbeReadingPressure:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    -1.f + (PhysicsProbePanelPressureBottomRight.x - static_cast<float>(lineExtent.width)) * mScreenToNdcX,
                    -1.f + PhysicsProbePanelPressureBottomRight.y * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::PhysicsProbeReadingSpeed:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    -1.f + (PhysicsProbePanelSpeedBottomRight.x - static_cast<float>(lineExtent.width)) * mScreenToNdcX,
                    -1.f + PhysicsProbePanelSpeedBottomRight.y * mScreenToNdcY);

                break;
            }

            case NotificationAnchorPositionType::PhysicsProbeReadingTemperature:
            {
                auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
                    textLine.Text.c_str(),
                    textLine.Text.length());

                linePositionNdc += vec2f(
                    -1.f + (PhysicsProbePanelTemperatureBottomRight.x - static_cast<float>(lineExtent.width)) * mScreenToNdcX,
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
        float const frameNdcWidth = static_cast<float>(frameSize.width) * mScreenToNdcX;
        float const frameNdcHeight = static_cast<float>(frameSize.height) * mScreenToNdcY;

        float const marginNdcWidth = MarginScreen * mScreenToNdcX;
        float const marginNdcHeight = MarginScreen * mScreenToNdcY;
        float const marginNdcHeightTop = MarginTopScreen * mScreenToNdcY;

        vec2f quadTopLeft( // Start with offset
            textureNotification.ScreenOffset.x * frameNdcWidth,
            -textureNotification.ScreenOffset.y * frameNdcHeight);

        switch (textureNotification.Anchor)
        {
            case AnchorPositionType::BottomLeft:
            {
                quadTopLeft += vec2f(
                    -1.f + marginNdcWidth,
                    -1.f + marginNdcHeight + frameNdcHeight);

                break;
            }

            case AnchorPositionType::BottomRight:
            {
                quadTopLeft += vec2f(
                    1.f - marginNdcWidth - frameNdcWidth,
                    -1.f + marginNdcHeight + frameNdcHeight);

                break;
            }

            case AnchorPositionType::TopLeft:
            {
                quadTopLeft += vec2f(
                    -1.f + marginNdcWidth,
                    1.f - marginNdcHeightTop);

                break;
            }

            case AnchorPositionType::TopRight:
            {
                quadTopLeft += vec2f(
                    1.f - marginNdcWidth - frameNdcWidth,
                    1.f - marginNdcHeightTop);

                break;
            }
        }

        vec2f quadBottomRight =
            quadTopLeft
            + vec2f(frameNdcWidth, -frameNdcHeight);

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