/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"

#include <cassert>
#include <cstdlib>

/*
 * This class is an OpenGL mapped buffer hidden behind a vector-like facade.
 */
template<typename TElement, GLenum TTarget>
class GameOpenGLMappedBuffer
{
public:

    GameOpenGLMappedBuffer()
        : mMappedBuffer(nullptr)
        , mSize(0u)
        , mAllocatedSize(0u)
    {
    }

    ~GameOpenGLMappedBuffer()
    {
        if (nullptr != mMappedBuffer)
        {
            unmap();
        }
    }

    void map(size_t size)
    {
        assert(nullptr == mMappedBuffer);

        if (size != 0)
        {
            mMappedBuffer = glMapBuffer(TTarget, GL_WRITE_ONLY);
            CheckOpenGLError();

            if (nullptr == mMappedBuffer)
            {
                throw GameException("glMapBuffer returned null pointer");
            }
        }

        mSize = 0u;
        mAllocatedSize = size;
    }

    void unmap()
    {
        // Might not be mapped in case the size was zero
        if (nullptr != mMappedBuffer)
        {
            glUnmapBuffer(TTarget);
            mMappedBuffer = nullptr;
        }

        // Leave size and allocated size as they are, as this
        // buffer may still be asked for its size (regardless
        // of whether or not its data has been uploaded)
    }

    template<typename... TArgs>
    inline TElement & emplace_back(TArgs&&... args)
    {
        assert(nullptr != mMappedBuffer);
        assert(mSize < mAllocatedSize);
        return *new(&(((TElement *)mMappedBuffer)[mSize++])) TElement(std::forward<TArgs>(args)...);
    }

    void reset()
    {
        assert(nullptr == mMappedBuffer);

        mSize = 0u;
        mAllocatedSize = 0u;
    }

    inline size_t size() const noexcept
    {
        return mSize;
    }

private:

    void * mMappedBuffer;
    size_t mSize;
    size_t mAllocatedSize;
};
