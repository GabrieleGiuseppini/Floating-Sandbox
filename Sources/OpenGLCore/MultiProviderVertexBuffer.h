/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-24
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"

#include <Core/BoundedVector.h>
#include <Core/Log.h>

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
    : mVBO()
    {
        for (size_t p = 0; p < NProviders; ++p)
        {
            mLastUploadedVertexCount[p] = 0;
            mDirtyProviders[p] = false;
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
        assert(mTotalVertexCount >= mVertexAttributesBuffer[iProvider].size());
        mTotalVertexCount -= mVertexAttributesBuffer[iProvider].size();

        // Clear and ensure size
        mVertexAttributesBuffer[iProvider].reset(nVertices);

        // Remember it's dirty
        mDirtyProviders[iProvider] = true;
    }

    template<typename TProvider>
    void UploadVertex(TProvider provider, TVertexAttributes const & vertexAttributes)
    {
        std::size_t const iProvider = static_cast<size_t>(provider);
        assert(iProvider < NProviders);

        mVertexAttributesBuffer[iProvider].emplace_back(vertexAttributes);
    }

    template<typename TProvider>
    size_t UploadEnd(TProvider provider)
    {
        std::size_t const iProvider = static_cast<size_t>(provider);
        assert(iProvider < NProviders);

        size_t const vertexCount = mVertexAttributesBuffer[iProvider].size();

        // Update total vertex count
        mTotalVertexCount += vertexCount;

        return vertexCount;
    }

    void RenderUpload()
    {
        //
        // First off, if we've grown we need to reallocate and thus need to rebuild buffer
        //

        if (mTotalVertexCount > mLastAllocatedVBOVertexCount)
        {
            // Just rebuild buffer, realloc VBO, and upload all

            LogMessage("*** TODOTEST: Rebuilding whole VBO");

            // Rebuild buffer
            mWorkBuffer.reset(mTotalVertexCount);
            for (size_t iProvider = 0; iProvider < NProviders; ++iProvider)
            {
                for (size_t v = 0; v < mVertexAttributesBuffer[iProvider].size(); ++v)
                {
                    mWorkBuffer.emplace_back(mVertexAttributesBuffer[iProvider][v]);
                }

                // Reset state
                mLastUploadedVertexCount[iProvider] = mVertexAttributesBuffer[iProvider].size();
                mDirtyProviders[iProvider] = false;
            }

            // Reallocate VBO and upload all
#ifndef MULTI_PROVIDER_VERTEX_BUFFER_TEST
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
        // Find prefix of non-dirty provider segments
        //

        size_t iFirstDirtyProvider;
        size_t prefixVertexCount = 0;
        for (iFirstDirtyProvider = 0; iFirstDirtyProvider < NProviders; ++iFirstDirtyProvider)
        {
            if (mDirtyProviders[iFirstDirtyProvider])
            {
                break;
            }

            prefixVertexCount += mVertexAttributesBuffer[iFirstDirtyProvider].size();
        }

        LogMessage("*** TODOTEST: FirstDirtyProvider=", iFirstDirtyProvider, " PrefixVertexCount=", prefixVertexCount);

        if (iFirstDirtyProvider == NProviders)
        {
            //
            // Nothing dirty, nothing to do
            //

            LogMessage("*** TODOTEST: No dirty, NOP");

            // No need to reset state: we're not dirty,
            // and number of uploaded vertices hasn't change for any provider

            return;
        }

        //
        // We are dirty from here
        //

        //
        // Process dirty provider segments
        //

        bool const isFirstDirtyProviderDirtyInSize = mVertexAttributesBuffer[iFirstDirtyProvider].size() != mLastUploadedVertexCount[iFirstDirtyProvider];

        if (isFirstDirtyProviderDirtyInSize
            && iFirstDirtyProvider < NProviders - 1)
        {
            //
            // This provider's size has changed and others follow, should re-upload everything from here
            //

            // TODO: this case seems to be mergeable with below

            InternalUploadAllFromProvider(iFirstDirtyProvider, prefixVertexCount);
        }
        else
        {
            //
            // This provider is dirty only in content, or dirty in size but last...
            //

            // Begin by rebuilding this buffer
            // TODO: merge in loop below, make it a while(true) and condition in middle
            assert(mWorkBuffer.size() >= mTotalVertexCount);
            size_t vd = prefixVertexCount;
            for (size_t vs = 0; vs < mVertexAttributesBuffer[iFirstDirtyProvider].size(); ++vs, ++vd)
            {
                mWorkBuffer[vd] = mVertexAttributesBuffer[iFirstDirtyProvider][vs];
            }

            // Reset state
            mLastUploadedVertexCount[iFirstDirtyProvider] = mVertexAttributesBuffer[iFirstDirtyProvider].size();
            mDirtyProviders[iFirstDirtyProvider] = false;

            // Continue with remaining
            bool isDirtyInSize = isFirstDirtyProviderDirtyInSize;
            for (size_t iProvider = iFirstDirtyProvider + 1; iProvider < NProviders; ++iProvider)
            {
                // This provider's buffer must be copied iff:
                //  - Any of the previous providers has changed size, OR
                //  - This provider is dirty (size or just content)
                if (isDirtyInSize || mDirtyProviders[iProvider])
                {
                    // Copy buffer
                    for (size_t vs = 0; vs < mVertexAttributesBuffer[iProvider].size(); ++vs, ++vd)
                    {
                        mWorkBuffer[vd] = mVertexAttributesBuffer[iProvider][vs];
                    }

                    // Reset state
                    mLastUploadedVertexCount[iProvider] = mVertexAttributesBuffer[iProvider].size();
                    mDirtyProviders[iProvider] = false;
                }

                isDirtyInSize |= (mVertexAttributesBuffer[iProvider].size() != mLastUploadedVertexCount[iProvider]);
            }

            // Upload dirty part

            LogMessage("*** TODOTEST: UploadDirtyPortion From=", prefixVertexCount, " Size=", vd - prefixVertexCount);

            InternalUploadToVBO(prefixVertexCount, vd - prefixVertexCount);
        }
    }

private:

    void InternalUploadAllFromProvider(size_t iProviderStart, size_t prefixVertexCount)
    {
        assert(iProviderStart < NProviders - 1);

        LogMessage("*** TODOTEST: UploadAll From=", iProviderStart, ", PrefixVertexCount=", prefixVertexCount);

        // Repopulate whole buffer from here
        assert(mWorkBuffer.size() >= mTotalVertexCount);
        size_t vd = prefixVertexCount;
        for (size_t iProvider = iProviderStart; iProvider < NProviders; ++iProvider)
        {
            for (size_t vs = 0; vs < mVertexAttributesBuffer[iProvider].size(); ++vs, ++vd)
            {
                mWorkBuffer[vd] = mVertexAttributesBuffer[iProvider][vs];
            }

            // Reset state
            mLastUploadedVertexCount[iProvider] = mVertexAttributesBuffer[iProvider].size();
            mDirtyProviders[iProvider] = false;
        }

        // Upload
        InternalUploadToVBO(prefixVertexCount, vd - prefixVertexCount);
    }

    void InternalUploadToVBO(size_t offset, size_t size)
    {
        assert(offset + size <= mTotalVertexCount);
        assert(offset + size <= mLastAllocatedVBOVertexCount);

        if (size > 0)
        {
#ifndef MULTI_PROVIDER_VERTEX_BUFFER_TEST
            glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(TVertexAttributes), size * sizeof(TVertexAttributes), &(mWorkBuffer.data()[offset]));
            CheckOpenGLError();
#else
            TestActions.push_back({
                TestAction::ActionKind::UploadVBO,
                offset * sizeof(TVertexAttributes),
                &(mWorkBuffer.data()[offset]),
                size * sizeof(TVertexAttributes) });
#endif
        }
    }

private:

    std::array<BoundedVector<TVertexAttributes>, NProviders> mVertexAttributesBuffer;
    std::array<size_t, NProviders> mLastUploadedVertexCount;
    std::array<bool, NProviders> mDirtyProviders;
    std::size_t mTotalVertexCount;

    BoundedVector<TVertexAttributes> mWorkBuffer; // For building vertex buffer; always mirror of actual vertex buffer

    GameOpenGLVBO mVBO;
    size_t mLastAllocatedVBOVertexCount; // only grows
};