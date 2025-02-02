/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GlobalRenderContext.h"

#include <Core/Noise.h>
#include <Core/TextureDatabase.h>

GlobalRenderContext::GlobalRenderContext(
    IAssetManager const & assetManager,
    ShaderManager<GameShaderSet::ShaderSet> & shaderManager)
    : mAssetManager(assetManager)
    , mShaderManager(shaderManager)
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

void GlobalRenderContext::InitializeNoiseTextures()
{
    //
    // Load noise texture database
    //

    auto noiseTextureDatabase = TextureDatabase<GameTextureDatabases::NoiseTextureDatabase>::Load(mAssetManager);

    //
    // Load noise frames
    //

    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();

    mUploadedNoiseTexturesManager.UploadFrame(
        NoiseType::Gross,
        noiseTextureDatabase.GetGroup(GameTextureDatabases::NoiseTextureGroups::Noise)
            .GetFrameSpecification(static_cast<TextureFrameIndex>(NoiseType::Gross))
            .LoadFrame(mAssetManager).TextureData,
        GL_RGBA,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        GL_LINEAR);

    mUploadedNoiseTexturesManager.UploadFrame(
        NoiseType::Fine,
        noiseTextureDatabase.GetGroup(GameTextureDatabases::NoiseTextureGroups::Noise)
            .GetFrameSpecification(static_cast<TextureFrameIndex>(NoiseType::Fine))
            .LoadFrame(mAssetManager).TextureData,
        GL_RGBA,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        GL_LINEAR);

    RegeneratePerlin_4_32_043_Noise(); // Will upload at firstRenderPrepare

    RegeneratePerlin_8_1024_073_Noise(); // Will upload at firstRenderPrepare
}

void GlobalRenderContext::InitializeGenericTextures()
{
    //
    // Create generic linear texture atlas
    //

    // Load texture database
    auto genericLinearTextureDatabase = TextureDatabase<GameTextureDatabases::GenericLinearTextureDatabase>::Load(mAssetManager);

    // Create atlas
    auto genericLinearTextureAtlas = TextureAtlasBuilder<GameTextureDatabases::GenericLinearTextureDatabase>::BuildAtlas(
        genericLinearTextureDatabase,
        TextureAtlasOptions::None,
        mAssetManager,
        [](float, ProgressMessageType) {});

    LogMessage("Generic linear texture atlas size: ", genericLinearTextureAtlas.Image.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::GenericLinearTexturesAtlasTexture>();

    // Create texture OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mGenericLinearTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericLinearTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(genericLinearTextureAtlas.Image));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mGenericLinearTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GameTextureDatabases::GenericLinearTextureDatabase>>(
        genericLinearTextureAtlas.Metadata);

    //
    // Flames
    //

    auto const & fireAtlasFrameMetadata = mGenericLinearTextureAtlasMetadata->GetFrameMetadata(GameTextureDatabases::GenericLinearTextureGroups::Fire, 0);

    vec2f const atlasPixelDx = vec2f(
        1.0f / static_cast<float>(mGenericLinearTextureAtlasMetadata->GetSize().width),
        1.0f / static_cast<float>(mGenericLinearTextureAtlasMetadata->GetSize().height));

    // Note: the below might not be perfect, as TextureCoordinatesBottomLeft includes dx, but TextureSpaceWidth/Height do not

    // Set FlamesBackground shader parameters
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::ShipFlamesBackground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::ShipFlamesBackground>();
    // Atlas tile coords, inclusive of extra pixel (for workaround to GL_LINEAR in atlas)
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::ShipFlamesBackground, GameShaderSet::ProgramParameterKind::AtlasTile1LeftBottomTextureCoordinates>(
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft + atlasPixelDx);
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::ShipFlamesBackground, GameShaderSet::ProgramParameterKind::AtlasTile1Size>(
        fireAtlasFrameMetadata.TextureSpaceWidth - atlasPixelDx.x * 2.0f,
        fireAtlasFrameMetadata.TextureSpaceHeight - atlasPixelDx.y * 2.0f);

    // Set FlamesForeground shader parameters
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::ShipFlamesForeground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::ShipFlamesForeground>();
    // Atlas tile coords, inclusive of extra pixel (for workaround to GL_LINEAR in atlas)
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::ShipFlamesForeground, GameShaderSet::ProgramParameterKind::AtlasTile1LeftBottomTextureCoordinates>(
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft + atlasPixelDx);
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::ShipFlamesForeground, GameShaderSet::ProgramParameterKind::AtlasTile1Size>(
        fireAtlasFrameMetadata.TextureSpaceWidth - atlasPixelDx.x * 2.0f,
        fireAtlasFrameMetadata.TextureSpaceHeight - atlasPixelDx.y * 2.0f);

    //
    // Create generic mipmapped texture atlas
    //

    // Load texture database
    auto genericMipMappedTextureDatabase = TextureDatabase<GameTextureDatabases::GenericMipMappedTextureDatabase>::Load(mAssetManager);

    // Create atlas
    auto genericMipMappedTextureAtlas = TextureAtlasBuilder<GameTextureDatabases::GenericMipMappedTextureDatabase>::BuildAtlas(
        genericMipMappedTextureDatabase,
        TextureAtlasOptions::MipMappable,
        mAssetManager,
        [](float, ProgressMessageType) {});

    LogMessage("Generic mipmapped texture atlas size: ", genericMipMappedTextureAtlas.Image.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::GenericMipMappedTexturesAtlasTexture>();

    // Create texture OpenGL handle
    glGenTextures(1, &tmpGLuint);
    mGenericMipMappedTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericMipMappedTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    assert(genericMipMappedTextureAtlas.Metadata.IsSuitableForMipMapping());
    GameOpenGL::UploadMipmappedAtlasTexture(
        std::move(genericMipMappedTextureAtlas.Image),
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
    mGenericMipMappedTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GameTextureDatabases::GenericMipMappedTextureDatabase>>(genericMipMappedTextureAtlas.Metadata);

    // Set texture in all shaders that use it
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::GenericMipMappedTexturesNdc>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::GenericMipMappedTexturesNdc>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::ShipGenericMipMappedTextures>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::ShipGenericMipMappedTextures>();
}

void GlobalRenderContext::InitializeExplosionTextures()
{
    // Load atlas
    auto explosionTextureAtlas = TextureAtlas<GameTextureDatabases::ExplosionTextureDatabase>::Deserialize(mAssetManager);

    LogMessage("Explosion texture atlas size: ", explosionTextureAtlas.Image.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::ExplosionsAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mExplosionTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mExplosionTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(explosionTextureAtlas.Image));

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
    mExplosionTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GameTextureDatabases::ExplosionTextureDatabase>>(explosionTextureAtlas.Metadata);

    // Set texture in ship shaders
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::ShipExplosions>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::ShipExplosions>();
}

void GlobalRenderContext::InitializeNpcTextures(TextureAtlas<GameTextureDatabases::NpcTextureDatabase> && npcTextureAtlas)
{
    LogMessage("NPC texture atlas size: ", npcTextureAtlas.Image.Size.ToString());

    // Activate texture
    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NpcAtlasTexture>();

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
        std::move(npcTextureAtlas.Image),
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
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::ShipNpcsTexture>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::ShipNpcsTexture>();
}

void GlobalRenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
{
    if (renderParameters.IsEffectiveAmbientLightIntensityDirty)
    {
        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::GenericMipMappedTexturesNdc>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::GenericMipMappedTexturesNdc, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
            renderParameters.EffectiveAmbientLightIntensity);
    }

    if (renderParameters.IsSkyDirty)
    {
        vec3f const effectiveMoonlightColor = renderParameters.EffectiveMoonlightColor.toVec3f();

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::GenericMipMappedTexturesNdc>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::GenericMipMappedTexturesNdc, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
            effectiveMoonlightColor);
    }
}

void GlobalRenderContext::RenderPrepareStart()
{
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

void GlobalRenderContext::RenderPrepareEnd()
{
    if (mElementIndices->IsDirty())
    {
        mElementIndices->Upload();
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
