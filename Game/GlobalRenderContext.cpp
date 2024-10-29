/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GlobalRenderContext.h"

#include "TextureDatabase.h"

#include <GameCore/Noise.h>

namespace Render {

GlobalRenderContext::GlobalRenderContext(ShaderManager<ShaderManagerTraits> & shaderManager)
    : mShaderManager(shaderManager)
    , mElementIndices(TriangleQuadElementArrayVBO::Create())
    // Textures
    , mGenericLinearTextureAtlasOpenGLHandle()
    , mGenericLinearTextureAtlasMetadata()
    , mGenericMipMappedTextureAtlasOpenGLHandle()
    , mGenericMipMappedTextureAtlasMetadata()
    , mExplosionTextureAtlasOpenGLHandle()
    , mExplosionTextureAtlasMetadata()
    , mNpcTextureAtlasOpenGLHandle()
    , mUploadedNoiseTexturesManager()
    , mPerlinNoise_4_32_043_ToUpload()
    , mPerlinNoise_8_1024_073_ToUpload()
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

    mUploadedNoiseTexturesManager.UploadFrame(
        NoiseType::Gross,
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise).GetFrameSpecification(static_cast<TextureFrameIndex>(NoiseType::Gross)).LoadFrame().TextureData,
        GL_RGBA,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        GL_LINEAR);

    mUploadedNoiseTexturesManager.UploadFrame(
        NoiseType::Fine,
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise).GetFrameSpecification(static_cast<TextureFrameIndex>(NoiseType::Fine)).LoadFrame().TextureData,
        GL_RGBA,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        GL_LINEAR);

    RegeneratePerlin_4_32_043_Noise(); // Will upload at firstRenderPrepare

    RegeneratePerlin_8_1024_073_Noise(); // Will upload at firstRenderPrepare
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
        AtlasOptions::MipMappable,
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
    assert(genericMipMappedTextureAtlas.Metadata.IsSuitableForMipMapping());
    GameOpenGL::UploadMipmappedAtlasTexture(
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

void GlobalRenderContext::InitializeNpcTextures(TextureAtlas<NpcTextureGroups> && npcTextureAtlas)
{
    LogMessage("NPC texture atlas size: ", npcTextureAtlas.AtlasData.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<ProgramParameterType::NpcAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mNpcTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mNpcTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    assert(npcTextureAtlas.Metadata.IsSuitableForMipMapping());
    GameOpenGL::UploadMipmappedAtlasTexture(
        std::move(npcTextureAtlas.AtlasData),
        npcTextureAtlas.Metadata.GetMaxDimension());

    // Set repeat mode - we want to clamp, to leverage the fact that
    // all frames are perfectly transparent at the edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Set texture in ship shaders
    mShaderManager.ActivateProgram<ProgramType::ShipNpcsTexture>();
    mShaderManager.SetTextureParameters<ProgramType::ShipNpcsTexture>();
}

void GlobalRenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
{
    if (renderParameters.IsEffectiveAmbientLightIntensityDirty)
    {
        mShaderManager.ActivateProgram<ProgramType::GenericMipMappedTexturesNdc>();
        mShaderManager.SetProgramParameter<ProgramType::GenericMipMappedTexturesNdc, ProgramParameterType::EffectiveAmbientLightIntensity>(
            renderParameters.EffectiveAmbientLightIntensity);
    }

    if (renderParameters.IsSkyDirty)
    {
        vec3f const effectiveMoonlightColor = renderParameters.EffectiveMoonlightColor.toVec3f();

        mShaderManager.ActivateProgram<ProgramType::GenericMipMappedTexturesNdc>();
        mShaderManager.SetProgramParameter<ProgramType::GenericMipMappedTexturesNdc, ProgramParameterType::EffectiveMoonlightColor>(
            effectiveMoonlightColor);
    }
}

void GlobalRenderContext::RenderPrepare()
{
    if (mElementIndices->IsDirty())
    {
        mElementIndices->Upload();
    }

    if (mPerlinNoise_4_32_043_ToUpload)
    {
        mUploadedNoiseTexturesManager.UploadFrame(
            NoiseType::Perlin_4_32_043,
            *mPerlinNoise_4_32_043_ToUpload,
            GL_R32F,
            GL_RED,
            GL_FLOAT,
            GL_LINEAR);

        mPerlinNoise_4_32_043_ToUpload.reset();
    }

    if (mPerlinNoise_8_1024_073_ToUpload)
    {
        mUploadedNoiseTexturesManager.UploadFrame(
            NoiseType::Perlin_8_1024_073,
            *mPerlinNoise_8_1024_073_ToUpload,
            GL_R32F,
            GL_RED,
            GL_FLOAT,
            GL_LINEAR);

        mPerlinNoise_8_1024_073_ToUpload.reset();
    }
}

void GlobalRenderContext::RegeneratePerlin_4_32_043_Noise()
{
    mPerlinNoise_4_32_043_ToUpload = MakePerlinNoise(
        IntegralRectSize(1024, 1024),
        4,
        32,
        0.43f);
}

void GlobalRenderContext::RegeneratePerlin_8_1024_073_Noise()
{
    mPerlinNoise_8_1024_073_ToUpload = MakePerlinNoise(
        IntegralRectSize(1024, 1024),
        8,
        1024,
        0.73f);
}

std::unique_ptr<Buffer2D<float, struct IntegralTag>> GlobalRenderContext::MakePerlinNoise(
    IntegralRectSize const & size,
    int firstGridDensity,
    int lastGridDensity,
    float persistence)
{
    auto buf = std::make_unique<Buffer2D<float, struct IntegralTag>>(
        Noise::CreateRepeatableFractal2DPerlinNoise(
            size,
            firstGridDensity,
            lastGridDensity,
            persistence));

    //
    // Scale values to 0, +1
    //

    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    for (size_t i = 0; i < buf->Size.GetLinearSize(); ++i)
    {
        minVal = std::min(minVal, buf->Data[i]);
        maxVal = std::max(maxVal, buf->Data[i]);
    }

    for (size_t i = 0; i < buf->Size.GetLinearSize(); ++i)
    {
        buf->Data[i] = (buf->Data[i] - minVal) / (maxVal - minVal);
    }

    return buf;
}

}