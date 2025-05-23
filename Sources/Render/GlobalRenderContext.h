/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameShaderSets.h"
#include "GameTextureDatabases.h"
#include "RenderParameters.h"

#include <OpenGLCore/GameOpenGL.h>
#include <OpenGLCore/ShaderManager.h>
#include <OpenGLCore/TriangleQuadElementArrayVBO.h>
#include <OpenGLCore/UploadedTextureManager.h>

#include <Core/Buffer2D.h>
#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/TextureAtlas.h>

#include <cassert>
#include <memory>

class GlobalRenderContext
{
public:

    GlobalRenderContext(
        IAssetManager const & assetManager,
        ShaderManager<GameShaderSets::ShaderSet> & shaderManager);

    ~GlobalRenderContext() = default;

    void InitializeNoiseTextures();

    void InitializeGenericTextures();

    void InitializeExplosionTextures();

    void InitializeNpcTextures(TextureAtlas<GameTextureDatabases::NpcTextureDatabase> && npcTextureAtlas);

    void ProcessParameterChanges(RenderParameters const & renderParameters);

    void RenderPrepareStart();

    void RenderPrepareEnd();

public:

    inline TriangleQuadElementArrayVBO & GetElementIndices()
    {
        assert(!!mElementIndices);
        return *mElementIndices;
    }

    inline TextureAtlasMetadata<GameTextureDatabases::GenericLinearTextureDatabase> const & GetGenericLinearTextureAtlasMetadata() const
    {
        assert(!!mGenericLinearTextureAtlasMetadata);
        return *mGenericLinearTextureAtlasMetadata;
    }

    inline GLuint GetGenericLinearTextureAtlasOpenGLHandle() const
    {
        assert(!!mGenericLinearTextureAtlasOpenGLHandle);
        return *mGenericLinearTextureAtlasOpenGLHandle;
    }

    inline TextureAtlasMetadata<GameTextureDatabases::GenericMipMappedTextureDatabase> const & GetGenericMipMappedTextureAtlasMetadata() const
    {
        assert(!!mGenericMipMappedTextureAtlasMetadata);
        return *mGenericMipMappedTextureAtlasMetadata;
    }

    inline TextureAtlasMetadata<GameTextureDatabases::ExplosionTextureDatabase> const & GetExplosionTextureAtlasMetadata() const
    {
        assert(!!mExplosionTextureAtlasMetadata);
        return *mExplosionTextureAtlasMetadata;
    }

    inline GLuint GetNoiseTextureOpenGLHandle(NoiseType noiseType) const
    {
        return mUploadedNoiseTexturesManager.GetOpenGLHandle(noiseType);
    }

    void RegeneratePerlin_4_32_043_Noise();

    void RegeneratePerlin_8_1024_073_Noise();

private:

    static std::unique_ptr<Buffer2D<float, struct IntegralTag>> MakePerlinNoise(
        IntegralRectSize const & size,
        int firstGridDensity,
        int lastGridDensity,
        float persistence);

private:

    IAssetManager const & mAssetManager;
    ShaderManager<GameShaderSets::ShaderSet> & mShaderManager;

    //
    // Global Element indices
    //

    std::unique_ptr<TriangleQuadElementArrayVBO> mElementIndices;

    //
    // Global Textures
    //

    GameOpenGLTexture mGenericLinearTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GameTextureDatabases::GenericLinearTextureDatabase>> mGenericLinearTextureAtlasMetadata;

    GameOpenGLTexture mGenericMipMappedTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GameTextureDatabases::GenericMipMappedTextureDatabase>> mGenericMipMappedTextureAtlasMetadata;

    GameOpenGLTexture mExplosionTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GameTextureDatabases::ExplosionTextureDatabase>> mExplosionTextureAtlasMetadata;

    GameOpenGLTexture mNpcTextureAtlasOpenGLHandle;

    UploadedTextureManager<NoiseType> mUploadedNoiseTexturesManager;
    std::unique_ptr<Buffer2D<float, struct IntegralTag>> mPerlinNoise_4_32_043_ToUpload; // When set, will be uploaded in rendering thread
    std::unique_ptr<Buffer2D<float, struct IntegralTag>> mPerlinNoise_8_1024_073_ToUpload; // When set, will be uploaded in rendering thread
};
