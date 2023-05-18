/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GlobalRenderContext.h"

#include "TextureDatabase.h"

namespace Render {

GlobalRenderContext::GlobalRenderContext(ShaderManager<ShaderManagerTraits> & shaderManager)
    : mShaderManager(shaderManager)
    // Textures
    , mGenericLinearTextureAtlasOpenGLHandle()
    , mGenericLinearTextureAtlasMetadata()
    , mGenericMipMappedTextureAtlasOpenGLHandle()
    , mGenericMipMappedTextureAtlasMetadata()
    , mExplosionTextureAtlasOpenGLHandle()
    , mExplosionTextureAtlasMetadata()
    , mUploadedNoiseTexturesManager()
{
}

GlobalRenderContext::~GlobalRenderContext()
{}

void GlobalRenderContext::InitializeNoiseTextures(ResourceLocator const & resourceLocator)
{
    //
    // Load noise texture database
    //

    auto noiseTextureDatabase = TextureDatabase<Render::NoiseTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    //
    // Load noise frames
    //

    mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture>();

    mUploadedNoiseTexturesManager.UploadNextFrame(
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise),
        0,
        GL_LINEAR);

    mUploadedNoiseTexturesManager.UploadNextFrame(
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise),
        1,
        GL_LINEAR);

    mUploadedNoiseTexturesManager.UploadNextFrame(
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise),
        2,
        GL_LINEAR);

    // TODOHERE
    /*
    //
    // Set noise 1 texture in shaders that require it
    //

    glBindTexture(GL_TEXTURE_2D, mUploadedNoiseTexturesManager.GetOpenGLHandle(NoiseTextureGroups::Noise, 0));
    CheckOpenGLError();

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground>();
    mShaderManager.SetTextureParameters<ProgramType::ShipFlamesBackground>();
    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground>();
    mShaderManager.SetTextureParameters<ProgramType::ShipFlamesForeground>();
    mShaderManager.ActivateProgram<ProgramType::ShipJetEngineFlames>();
    mShaderManager.SetTextureParameters<ProgramType::ShipJetEngineFlames>();

    //
    // Load noise 2
    //

    mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture2>();

    mUploadedNoiseTexturesManager.UploadNextFrame(
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise),
        1,
        GL_LINEAR);

    //
    // Set noise 2 texture in shaders that require it
    //

    glBindTexture(GL_TEXTURE_2D, mUploadedNoiseTexturesManager.GetOpenGLHandle(NoiseTextureGroups::Noise, 1));
    CheckOpenGLError();

    mShaderManager.ActivateProgram<ProgramType::LaserRay>();
    mShaderManager.SetTextureParameters<ProgramType::LaserRay>();

    //
    // Load noise 3
    //

    mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture3>();

    mUploadedNoiseTexturesManager.UploadNextFrame(
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise),
        2,
        GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, mUploadedNoiseTexturesManager.GetOpenGLHandle(NoiseTextureGroups::Noise, 2));
    CheckOpenGLError();
    */
}

void GlobalRenderContext::InitializeGenericTextures(ResourceLocator const & resourceLocator)
{
    //
    // Create generic linear texture atlas
    //

    // Load texture database
    auto genericLinearTextureDatabase = TextureDatabase<Render::GenericLinearTextureTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Create atlas
    auto genericLinearTextureAtlas = TextureAtlasBuilder<GenericLinearTextureGroups>::BuildAtlas(
        genericLinearTextureDatabase,
        AtlasOptions::None,
        [](float, ProgressMessageType) {});

    LogMessage("Generic linear texture atlas size: ", genericLinearTextureAtlas.AtlasData.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<ProgramParameterType::GenericLinearTexturesAtlasTexture>();

    // Create texture OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mGenericLinearTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericLinearTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(genericLinearTextureAtlas.AtlasData));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mGenericLinearTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GenericLinearTextureGroups>>(
        genericLinearTextureAtlas.Metadata);

    //
    // Flames
    //

    // Set FlamesBackground shader parameters
    auto const & fireAtlasFrameMetadata = mGenericLinearTextureAtlasMetadata->GetFrameMetadata(GenericLinearTextureGroups::Fire, 0);
    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground>();
    mShaderManager.SetTextureParameters<ProgramType::ShipFlamesBackground>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground, ProgramParameterType::AtlasTile1Dx>(
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.width),
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.height));
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground, ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates>(
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft);
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground, ProgramParameterType::AtlasTile1Size>(
        fireAtlasFrameMetadata.TextureSpaceWidth,
        fireAtlasFrameMetadata.TextureSpaceHeight);

    // Set FlamesForeground shader parameters
    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground>();
    mShaderManager.SetTextureParameters<ProgramType::ShipFlamesForeground>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground, ProgramParameterType::AtlasTile1Dx>(
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.width),
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.height));
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground, ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates>(
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft);
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground, ProgramParameterType::AtlasTile1Size>(
        fireAtlasFrameMetadata.TextureSpaceWidth,
        fireAtlasFrameMetadata.TextureSpaceHeight);

    //
    // Create generic mipmapped texture atlas
    //

    // Load texture database
    auto genericMipMappedTextureDatabase = TextureDatabase<Render::GenericMipMappedTextureTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Create atlas
    auto genericMipMappedTextureAtlas = TextureAtlasBuilder<GenericMipMappedTextureGroups>::BuildAtlas(
        genericMipMappedTextureDatabase,
        AtlasOptions::None,
        [](float, ProgressMessageType) {});

    LogMessage("Generic mipmapped texture atlas size: ", genericMipMappedTextureAtlas.AtlasData.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<ProgramParameterType::GenericMipMappedTexturesAtlasTexture>();

    // Create texture OpenGL handle
    glGenTextures(1, &tmpGLuint);
    mGenericMipMappedTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericMipMappedTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadMipmappedPowerOfTwoTexture(
        std::move(genericMipMappedTextureAtlas.AtlasData),
        genericMipMappedTextureAtlas.Metadata.GetMaxDimension());

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mGenericMipMappedTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GenericMipMappedTextureGroups>>(genericMipMappedTextureAtlas.Metadata);

    // Set texture in all shaders that use it
    mShaderManager.ActivateProgram<ProgramType::GenericMipMappedTexturesNdc>();
    mShaderManager.SetTextureParameters<ProgramType::GenericMipMappedTexturesNdc>();
    mShaderManager.ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();
    mShaderManager.SetTextureParameters<ProgramType::ShipGenericMipMappedTextures>();
}

void GlobalRenderContext::InitializeExplosionTextures(ResourceLocator const & resourceLocator)
{
    // Load atlas
    TextureAtlas<ExplosionTextureGroups> explosionTextureAtlas = TextureAtlas<ExplosionTextureGroups>::Deserialize(
        ExplosionTextureDatabaseTraits::DatabaseName,
        resourceLocator.GetTexturesRootFolderPath());

    LogMessage("Explosion texture atlas size: ", explosionTextureAtlas.AtlasData.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<ProgramParameterType::ExplosionsAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mExplosionTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mExplosionTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(explosionTextureAtlas.AtlasData));

    // Set repeat mode - we want to clamp, to leverage the fact that
    // all frames are perfectly transparent at the edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mExplosionTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<ExplosionTextureGroups>>(explosionTextureAtlas.Metadata);

    // Set texture in ship shaders
    mShaderManager.ActivateProgram<ProgramType::ShipExplosions>();
    mShaderManager.SetTextureParameters<ProgramType::ShipExplosions>();
}

void GlobalRenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
{
    if (renderParameters.IsEffectiveAmbientLightIntensityDirty)
    {
        mShaderManager.ActivateProgram<ProgramType::GenericMipMappedTexturesNdc>();
        mShaderManager.SetProgramParameter<ProgramType::GenericMipMappedTexturesNdc, ProgramParameterType::EffectiveAmbientLightIntensity>(
            renderParameters.EffectiveAmbientLightIntensity);
    }
}


}