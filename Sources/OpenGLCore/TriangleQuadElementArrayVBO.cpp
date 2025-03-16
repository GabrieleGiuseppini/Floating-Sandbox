/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2024-04-7
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TriangleQuadElementArrayVBO.h"

std::unique_ptr<TriangleQuadElementArrayVBO> TriangleQuadElementArrayVBO::Create()
{
	// Create VBO
	GLuint tmpGLuint;
	glGenBuffers(1, &tmpGLuint);
	GameOpenGLVBO vbo(tmpGLuint);

	return std::unique_ptr<TriangleQuadElementArrayVBO>(
		new TriangleQuadElementArrayVBO(
			std::move(vbo)));
}

void TriangleQuadElementArrayVBO::Grow(size_t quadCount)
{
	assert(quadCount > mQuadCount);

	// Enlarge buffer
	size_t requiredSize = quadCount * 6; // 6 indices per quad
	mIndices.ensure_size(requiredSize);

	// Add new indices
	int indexStart = static_cast<int>(mQuadCount) * 4;
	for (size_t q = mQuadCount; q < quadCount; ++q, indexStart += 4)
	{
		mIndices.emplace_back(indexStart);
		mIndices.emplace_back(indexStart + 1);
		mIndices.emplace_back(indexStart + 2);
		mIndices.emplace_back(indexStart + 1);
		mIndices.emplace_back(indexStart + 2);
		mIndices.emplace_back(indexStart + 3);
	}

	mQuadCount = quadCount;

	// Remember we're dirty now
	mIsDirty = true;
}