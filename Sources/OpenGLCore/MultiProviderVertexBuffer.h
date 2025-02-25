/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-24
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"

#include <Core/BoundedVector.h>

#include <array>
#include <cassert>
#include <cstring>

#ifdef MULTI_PROVIDER_VERTEX_BUFFER_TEST
#include <vector>
#endif

/*
 * A vertex buffer that is contributed to by different, independent actors ("providers") that
 * do not talk to each other.
 *
 * Vertex attributed uploaded by each provider are sticky.
 */
template<typename TVertexAttributes, size_t NProviders>
class MultiProviderVertexBuffer
{
#ifndef MULTI_PROVIDER_VERTEX_BUFFER_TEST
    static_assert(NProviders > 1, "Do not use MultiProviderVertexBuffer for just one provider");
#endif

#ifdef MULTI_PROVIDER_VERTEX_BUFFER_TEST
public:

    struct TestAction
    {
        enum class ActionKind
        {
            AllocateAndUploadVBO,
            UploadVBO
        };

        ActionKind Action;
        size_t Offset;
        TVertexAttributes const * Pointer;
        size_t Size;
    };

    std::vector<TestAction> TestActions;
#endif

public:

    MultiProviderVertexBuffer()
        : mIsGlobalDirty(false)
        , mVBO()
    {
        for (size_t p = 0; p < NProviders; ++p)
        {
            mProviderData[p].LastUploadedVertexCount = 0;
            mProviderData[p].IsDirty = false;
        }

        mTotalVertexCount = 0;
        mLastAllocatedVBOVertexCount = 0;

#ifndef MULTI_PROVIDER_VERTEX_BUFFER_TEST

        // Initialize VBO
        GLuint tmpGLuint;
        glGenBuffers(1, &tmpGLuint);
        mVBO = tmpGLuint;

#endif
    }

    std::size_t GetTotalVertexCount() const
    {
        return mTotalVertexCount;
    }

    bool IsEmpty() const
    {
        return mTotalVertexCount == 0;
    }

    void Bind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mVBO);
    }

    template<typename TProvider>
    void UploadStart(TProvider provider, size_t nVertices)
    {
        std::size_t const iProvider = static_cast<size_t>(provider);
        assert(iProvider < NProviders);

        // Update total vertex count
        assert(mTotalVertexCount >= mProviderData[iProvider].VertexAttributesBuffer.size());
        mTotalVertexCount -= mProviderData[iProvider].VertexAttributesBuffer.size();

        // Clear provider's buffer and ensure size
        mProviderData[iProvider].VertexAttributesBuffer.reset(nVertices);
    }

    template<typename TProvider>
    void UploadVertex(TProvider provider, TVertexAttributes const & vertexAttributes)
    {
        std::size_t const iProvider = static_cast<size_t>(provider);
        assert(iProvider < NProviders);

        mProviderData[iProvider].VertexAttributesBuffer.emplace_back(vertexAttributes);
    }

    template<typename TProvider>
    size_t UploadEnd(TProvider provider)
    {
        std::size_t const iProvider = static_cast<size_t>(provider);
        assert(iProvider < NProviders);

        // Remember provider is dirty
        mProviderData[iProvider].IsDirty = true;
        mIsGlobalDirty = true;

        // Update total vertex count
        size_t const vertexCount = mProviderData[iProvider].VertexAttributesBuffer.size();
        mTotalVertexCount += vertexCount;

        return vertexCount;
    }

    void RenderUpload()
    {
        //
        // We expect to be not dirty most of the times, hence this check is worth it
        //

        if (!mIsGlobalDirty)
        {
            return;
        }

        mIsGlobalDirty = false; // A bit too early, but will be true when we're done

        //
        // First off, if we've grown we need to reallocate and thus need to rebuild buffer
        //

        if (mTotalVertexCount > mLastAllocatedVBOVertexCount)
        {
            // Just rebuild buffer, realloc VBO, and upload all

            // Rebuild buffer
            mWorkBuffer.reset(mTotalVertexCount);
            for (size_t iProvider = 0; iProvider < NProviders; ++iProvider)
            {
                auto & providerData = mProviderData[iProvider];
                size_t providerBufferSize = providerData.VertexAttributesBuffer.size();

                for (size_t v = 0; v < providerBufferSize; ++v)
                {
                    mWorkBuffer.emplace_back(providerData.VertexAttributesBuffer[v]);
                }

                // Reset state
                providerData.LastUploadedVertexCount = providerBufferSize;
                providerData.IsDirty = false;
            }

            // Reallocate VBO and upload all
#ifndef MULTI_PROVIDER_VERTEX_BUFFER_TEST
            Bind();
            glBufferData(GL_ARRAY_BUFFER, mTotalVertexCount * sizeof(TVertexAttributes), mWorkBuffer.data(), GL_DYNAMIC_DRAW);
            CheckOpenGLError();
#else
            TestActions.push_back({
                TestAction::ActionKind::AllocateAndUploadVBO,
                0u,
                mWorkBuffer.data(),
                mTotalVertexCount * sizeof(TVertexAttributes) });
#endif

            mLastAllocatedVBOVertexCount = mTotalVertexCount;

            return;
        }

        //
        // Required size has not grown - from here on we won't realloc VBO,
        // nor rebuild work buffer unless where needed
        //

        assert(mWorkBuffer.size() >= mTotalVertexCount);

        bool isDirty = false;
        bool forceRebuild = false;
        size_t iWb = 0; // Offset in work buffer that we write to
        size_t iDirtyStartWb = 0; // Offset in work buffer where dirty starts - anything before this is clean
        size_t iDirtyEndWb = 0; // Offset in work buffer where dirty ends - anything after this is clean

        for (size_t iProvider = 0; iProvider < NProviders; ++iProvider)
        {
            auto & providerData = mProviderData[iProvider];

            size_t const providerBufferSize = providerData.VertexAttributesBuffer.size();

            if (providerData.IsDirty || forceRebuild)
            {
                // Rebuild work buffer portion for this provider

                if (!isDirty)
                {
                    // Remember where the dirty portion starts
                    iDirtyStartWb = iWb;

                    // From now on we're dirty
                    isDirty = true;
                }

                // Copy in-place
                mWorkBuffer.copy_from(providerData.VertexAttributesBuffer, iWb, providerBufferSize);
                iWb += providerBufferSize;

                // Remember (current) end of dirty portion
                iDirtyEndWb = iWb;

                // If size has changed, we'll need to keep rebuilding
                forceRebuild |= (providerBufferSize != providerData.LastUploadedVertexCount);

                // Reset state for provider
                providerData.LastUploadedVertexCount = providerBufferSize;
                providerData.IsDirty = false;
            }
            else
            {
                // Advance work buffer, leaving what's there
                iWb += providerBufferSize;

                // No need to update last uploaded vertex count - no change
            }
        }

        if (iDirtyEndWb > iDirtyStartWb)
        {
#ifndef MULTI_PROVIDER_VERTEX_BUFFER_TEST
            Bind();
            glBufferSubData(GL_ARRAY_BUFFER, iDirtyStartWb * sizeof(TVertexAttributes), (iDirtyEndWb - iDirtyStartWb) * sizeof(TVertexAttributes), &(mWorkBuffer.data()[iDirtyStartWb]));
            CheckOpenGLError();
#else
            TestActions.push_back({
                TestAction::ActionKind::UploadVBO,
                iDirtyStartWb * sizeof(TVertexAttributes),
                &(mWorkBuffer.data()[iDirtyStartWb]),
                (iDirtyEndWb - iDirtyStartWb) * sizeof(TVertexAttributes) });
#endif
        }
    }

private:

    struct ProviderData
    {
        BoundedVector<TVertexAttributes> VertexAttributesBuffer;
        size_t LastUploadedVertexCount;
        bool IsDirty;
    };

    std::array<ProviderData, NProviders> mProviderData;
    bool mIsGlobalDirty; // Set when at least one provider is dirty
    std::size_t mTotalVertexCount; // Total number of vertices, valid at beginning of RenderUpload()

    BoundedVector<TVertexAttributes> mWorkBuffer; // For building single vertex buffer; always mirror of actual VBO

    GameOpenGLVBO mVBO;
    size_t mLastAllocatedVBOVertexCount; // only grows
};