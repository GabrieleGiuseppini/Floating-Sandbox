/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderParameters.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>
#include <GameOpenGL/TriangleQuadElementArrayVBO.h>
#include <GameOpenGL/UploadedTextureManager.h>

#include <GameCore/Buffer2D.h>

#include <cassert>
#include <memory>

namespace Render {

class GlobalRenderContext
{
public:

    GlobalRenderContext(ShaderManager<ShaderManagerTraits> & shaderManager);

    ~GlobalRenderContext();

    void InitializeNoiseTextures(ResourceLocator const & resourceLocator);

    void InitializeGenericTextures(ResourceLocator const & resourceLocator);

    void InitializeExplosionTextures(ResourceLocator const & resourceLocator);

    void InitializeNpcTextures(TextureAtlas<NpcTextureGroups> && npcTextureAtlas);

    void ProcessParameterChanges(RenderParameters const & renderParameters);

    void RenderPrepareStart();

    void RenderPrepareEnd();

public:

    inline TriangleQuadElementArrayVBO & GetElementIndices()
    {
        assert(!!mElementIndices);
        return *mElementIndices;
    }

    inline TextureAtlasMetadata<GenericLinearTextureGroups> const & GetGenericLinearTextureAtlasMetadata() const
    {
        assert(!!mGenericLinearTextureAtlasMetadata);
        return *mGenericLinearTextureAtlasMetadata;
    }

    inline GLuint GetGenericLinearTextureAtlasOpenGLHandle() const
    {
        assert(!!mGenericLinearTextureAtlasOpenGLHandle);
        return *mGenericLinearTextureAtlasOpenGLHandle;
    }

    inline TextureAtlasMetadata<GenericMipMappedTextureGroups> const & GetGenericMipMappedTextureAtlasMetadata() const
    {
        assert(!!mGenericMipMappedTextureAtlasMetadata);
        return *mGenericMipMappedTextureAtlasMetadata;
    }

    inline TextureAtlasMetadata<ExplosionTextureGroups> const & GetExplosionTextureAtlasMetadata() const
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

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    //
    // Global Element indices
    //

    std::unique_ptr<TriangleQuadElementArrayVBO> mElementIndices;

    //
    // Global Textures
    //

    GameOpenGLTexture mGenericLinearTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericLinearTextureGroups>> mGenericLinearTextureAtlasMetadata;

    GameOpenGLTexture mGenericMipMappedTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericMipMappedTextureGroups>> mGenericMipMappedTextureAtlasMetadata;

    GameOpenGLTexture mExplosionTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<ExplosionTextureGroups>> mExplosionTextureAtlasMetadata;

    GameOpenGLTexture mNpcTextureAtlasOpenGLHandle;

    UploadedTextureManager<NoiseType> mUploadedNoiseTexturesManager;
    std::unique_ptr<Buffer2D<float, struct IntegralTag>> mPerlinNoise_4_32_043_ToUpload; // When set, will be uploaded in rendering thread
    std::unique_ptr<Buffer2D<float, struct IntegralTag>> mPerlinNoise_8_1024_073_ToUpload; // When set, will be uploaded in rendering thread
};

}