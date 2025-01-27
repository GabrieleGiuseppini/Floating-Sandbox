/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2024-04-7
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"

#include <Core/BoundedVector.h>

#include <cassert>
#include <memory>

/*
 * Wraps a ELEMENT_ARRAY_BUFFER VBO with indices for making two-triangle quads out of four
 * vertices, assumed to be laid out as follows:
 *
 *  A C
 *  |/|
 *  B D
 *
 */
class TriangleQuadElementArrayVBO final
{
public:

	static std::unique_ptr<TriangleQuadElementArrayVBO> Create();

	void EnsureSize(size_t quadCount)
	{
		if (quadCount > mQuadCount)
		{
			Grow(quadCount);
		}
	}

	bool IsDirty() const
	{
		return mIsDirty;
	}

	void Bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mVBO);
	}

	void Upload()
	{
		assert(mIsDirty);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mVBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(int), mIndices.data(), GL_STREAM_DRAW);
		CheckOpenGLError();

		mIsDirty = false;
	}

private:

	explicit TriangleQuadElementArrayVBO(GameOpenGLVBO && vbo)
		: mIndices()
		, mQuadCount(0)
		, mVBO(std::move(vbo))
		, mIsDirty(false)
	{

	}

	void Grow(size_t quadCount);

private:

	// The (vertex) indices
	BoundedVector<int> mIndices;

	// Number of quads (consistent with number of indices)
	size_t mQuadCount;

	// Our VBO
	GameOpenGLVBO mVBO;

	// Whether or not the indices must be re-uploaded to the GPU
	bool mIsDirty;
};
