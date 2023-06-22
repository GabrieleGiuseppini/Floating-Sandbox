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
    , mIsTextureNotificationDataDirty(false)
    , mTextureNotificationVAO()
    , mTextureNotificationVertexBuffer()
    , mTextureNotificationVBO()
    // Physics probe panel
    , mPhysicsProbePanel()
    , mIsPhysicsProbeDataDirty(false)
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
    , mBlastToolHaloVAO()
    , mBlastToolHaloVBO()
    , mPressureInjectionHaloVAO()
    , mPressureInjectionHaloVBO()
    , mWindSphereVAO()
    , mWindSphereVBO()
    , mLaserCannonVAO()
    , mLaserCannonVBO()
    , mLaserRayVAO()
    , mLaserRayVBO()
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

    glBindVertexArray(0);

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
    // Initialize Blast Tool halo
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mBlastToolHaloVAO = tmpGLuint;

        glBindVertexArray(*mBlastToolHaloVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mBlastToolHaloVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(BlastToolHaloVertex) == (4 + 2) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mBlastToolHaloVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::BlastToolHalo1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::BlastToolHalo1), 4, GL_FLOAT, GL_FALSE, sizeof(BlastToolHaloVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::BlastToolHalo2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::BlastToolHalo2), 2, GL_FLOAT, GL_FALSE, sizeof(BlastToolHaloVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);

        // Set texture parameters
        mShaderManager.ActivateProgram<ProgramType::BlastToolHalo>();
        mShaderManager.SetTextureParameters<ProgramType::BlastToolHalo>();
    }

    //
    // Initialize Pressure Injection halo
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mPressureInjectionHaloVAO = tmpGLuint;

        glBindVertexArray(*mPressureInjectionHaloVAO);
        CheckOpenGLError();

        glGenBuffers(1, &tmpGLuint);
        mPressureInjectionHaloVBO = tmpGLuint;

        // Describe vertex attributes
        static_assert(sizeof(PressureInjectionHaloVertex) == (4 + 1) * sizeof(float));
        glBindBuffer(GL_ARRAY_BUFFER, *mPressureInjectionHaloVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::PressureInjectionHalo1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::PressureInjectionHalo1), 4, GL_FLOAT, GL_FALSE, sizeof(PressureInjectionHaloVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::PressureInjectionHalo2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::PressureInjectionHalo2), 1, GL_FLOAT, GL_FALSE, sizeof(PressureInjectionHaloVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
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

        // Set texture parameters
        mShaderManager.ActivateProgram<ProgramType::LaserRay>();
        mShaderManager.SetTextureParameters<ProgramType::LaserRay>();
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

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void NotificationRenderContext::UploadStart()
{
    // Reset HeatBlaster flame, it's uploaded as needed
    mHeatBlasterFlameShaderToRender.reset();

    // Reset fire extinguisher spray, it's uploaded as needed
    mFireExtinguisherSprayShaderToRender.reset();

    // Reset blast tool halo, it's uploaded as needed
    mBlastToolHaloVertexBuffer.clear();

    // Reset pressure injection halo, it's uploaded as needed
    mPressureInjectionHaloVertexBuffer.clear();

    // Reset wind sphere, it's uploaded as needed
    mWindSphereVertexBuffer.clear();

    // Reset laser cannon, it's uploaded as needed
    mLaserCannonVertexBuffer.clear();

    // Reset laser ray, it's uploaded as needed
    mLaserRayVertexBuffer.clear();
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

    RenderPrepareBlastToolHalo();

    RenderPreparePressureInjectionHalo();

    RenderPrepareWindSphere();

    RenderPrepareLaserCannon();

    RenderPrepareLaserRay();
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

    RenderDrawBlastToolHalo();

    RenderDrawPressureInjectionHalo();

    RenderDrawWindSphere();
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

    mShaderManager.ActivateProgram<ProgramType::BlastToolHalo>();
    mShaderManager.SetProgramParameter<ProgramType::BlastToolHalo, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::PressureInjectionHalo>();
    mShaderManager.SetProgramParameter<ProgramType::PressureInjectionHalo, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::WindSphere>();
    mShaderManager.SetProgramParameter<ProgramType::WindSphere, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);
}

void NotificationRenderContext::ApplyCanvasSizeChanges(RenderParameters const & renderParameters)
{
    // TODO: we'd need to recalculate the physics panel probe panel vertices,
    // as the pixel size of the panel is constant and thus its NDC size is changing

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
    mIsTextureNotificationDataDirty = true;
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
            float const textureWidthNdc = atlasFrame.TextureCoordinatesTopRight.x - atlasFrame.TextureCoordinatesBottomLeft.x;
            float const textureHeightNdc = atlasFrame.TextureCoordinatesTopRight.y - atlasFrame.TextureCoordinatesBottomLeft.y;

            // Triangle 1

            // Top-left
            mPhysicsProbePanelVertexBuffer.emplace_back(
                quadTopLeft,
                vec2f(0.0f, textureHeightNdc),
                xLimits,
                opening);

            // Top-right
            mPhysicsProbePanelVertexBuffer.emplace_back(
                vec2f(quadBottomRight.x, quadTopLeft.y),
                vec2f(textureWidthNdc, textureHeightNdc),
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
                vec2f(textureWidthNdc, textureHeightNdc),
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
                vec2f(textureWidthNdc, 0.0f),
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

void NotificationRenderContext::RenderPrepareBlastToolHalo()
{
    if (!mBlastToolHaloVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mBlastToolHaloVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(BlastToolHaloVertex) * mBlastToolHaloVertexBuffer.size(),
            mBlastToolHaloVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void NotificationRenderContext::RenderDrawBlastToolHalo()
{
    if (!mBlastToolHaloVertexBuffer.empty())
    {
        glBindVertexArray(*mBlastToolHaloVAO);

        mShaderManager.ActivateProgram<ProgramType::BlastToolHalo>();

        // Setup blending
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        // Draw
        assert((mBlastToolHaloVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mBlastToolHaloVertexBuffer.size()));

        // Reset blending
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        glBindVertexArray(0);
    }
}

void NotificationRenderContext::RenderPreparePressureInjectionHalo()
{
    if (!mPressureInjectionHaloVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mPressureInjectionHaloVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(PressureInjectionHaloVertex) * mPressureInjectionHaloVertexBuffer.size(),
            mPressureInjectionHaloVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Set time parameter
        mShaderManager.ActivateProgram<ProgramType::PressureInjectionHalo>();
        mShaderManager.SetProgramParameter<ProgramParameterType::Time>(
            ProgramType::PressureInjectionHalo,
            GameWallClock::GetInstance().NowAsFloat());
    }
}

void NotificationRenderContext::RenderDrawPressureInjectionHalo()
{
    if (!mPressureInjectionHaloVertexBuffer.empty())
    {
        glBindVertexArray(*mPressureInjectionHaloVAO);

        mShaderManager.ActivateProgram<ProgramType::PressureInjectionHalo>();

        // Setup blending
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        // Draw
        assert((mPressureInjectionHaloVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mPressureInjectionHaloVertexBuffer.size()));

        // Reset blending
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

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

void NotificationRenderContext::GeneratePhysicsProbePanelVertices()
{
    // TODOHERE
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
            textureNotification.ScreenOffset.x * static_cast<float>(frameSize.width) * mScreenToNdcX,
            -textureNotification.ScreenOffset.y * static_cast<float>(frameSize.height) * mScreenToNdcY);

        switch (textureNotification.Anchor)
        {
            case AnchorPositionType::BottomLeft:
            {
                quadTopLeft += vec2f(
                    -1.f + MarginScreen * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(frameSize.height)) * mScreenToNdcY);

                break;
            }

            case AnchorPositionType::BottomRight:
            {
                quadTopLeft += vec2f(
                    1.f - (MarginScreen + static_cast<float>(frameSize.width)) * mScreenToNdcX,
                    -1.f + (MarginScreen + static_cast<float>(frameSize.height)) * mScreenToNdcY);

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
                    1.f - (MarginScreen + static_cast<float>(frameSize.width)) * mScreenToNdcX,
                    1.f - MarginTopScreen * mScreenToNdcY);

                break;
            }
        }

        vec2f quadBottomRight = quadTopLeft + vec2f(
            static_cast<float>(frameSize.width) * mScreenToNdcX,
            -static_cast<float>(frameSize.height) * mScreenToNdcY);

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