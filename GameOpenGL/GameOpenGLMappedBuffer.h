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

        mMappedBuffer = glMapBuffer(TTarget, GL_WRITE_ONLY);
        CheckOpenGLError();

        mSize = 0u;
        mAllocatedSize = size;
    }

    void unmap()
    {
        assert(nullptr != mMappedBuffer);

        glUnmapBuffer(TTarget);
        mMappedBuffer = nullptr;
    }

    template<typename... TArgs>
    inline TElement & emplace_back(TArgs&&... args)
    {
        assert(nullptr != mMappedBuffer);
        assert(mSize < mAllocatedSize);
        return *new(&(((TElement *)mMappedBuffer)[mSize++])) TElement(std::forward<TArgs>(args)...);
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
