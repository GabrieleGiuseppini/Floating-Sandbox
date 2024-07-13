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
#include "UploadedTextureManager.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>
#include <GameOpenGL/TriangleQuadElementArrayVBO.h>

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

    void RenderPrepare();

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
        return mUploadedNoiseTexturesManager.GetOpenGLHandle(NoiseTextureGroups::Noise, static_cast<TextureFrameIndex>(noiseType));
    }

private:

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    //
    // Element indices
    //

    std::unique_ptr<TriangleQuadElementArrayVBO> mElementIndices;

    //
    // Textures
    //

    GameOpenGLTexture mGenericLinearTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericLinearTextureGroups>> mGenericLinearTextureAtlasMetadata;

    GameOpenGLTexture mGenericMipMappedTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericMipMappedTextureGroups>> mGenericMipMappedTextureAtlasMetadata;

    GameOpenGLTexture mExplosionTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<ExplosionTextureGroups>> mExplosionTextureAtlasMetadata;

    UploadedTextureManager<NoiseTextureGroups> mUploadedNoiseTexturesManager;
};

}