/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2024-09-01
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Noise.h"

#include "GameRandomEngine.h"
#include "Log.h"

#include <cassert>

Buffer2D<float, struct IntegralTag> Noise::CreateRepeatableFractal2DPerlinNoise(
	IntegralRectSize const & size,
	int firstGridDensity, // Number of cells
	int lastGridDensity, // Number of cells
	float persistence)
{
	//
	// Create float buf
	//

	auto floatBuf = Buffer2D<float, struct IntegralTag>(size);
	floatBuf.Fill(0.0f);

	//
	// Run for all grids
	//

	float amplitude = 1.0f;

	firstGridDensity = std::min(std::min(firstGridDensity, size.width), size.height);
	lastGridDensity = std::min(std::min(lastGridDensity, size.width), size.height);

	for (int gridDensity = firstGridDensity; ; )
	{
		//
		// Add new Perlin
		//

		AddRepeatableUnscaledPerlinNoise(
			floatBuf,
			gridDensity,
			amplitude);

		//
		// Advance
		//

		if (gridDensity == lastGridDensity)
			break;

		if (gridDensity < lastGridDensity)
			gridDensity *= 2;
		else
			gridDensity /= 2;

		amplitude *= persistence;
	}

	return floatBuf;
}

Buffer2D<vec2f, struct IntegralTag> Noise::MakePerlinVectorGrid(IntegralRectSize const & size)
{
	auto grid = Buffer2D<vec2f, struct IntegralTag>(size);

	for (int y = 0; y < size.height; ++y)
	{
		for (int x = 0; x < size.width; ++x)
		{
			grid[{x, y}] = vec2f(
				GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, 1.0f),
				GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, 1.0f))
				.normalise();
		}
	}

	return grid;
}

void Noise::AddRepeatableUnscaledPerlinNoise(
	Buffer2D<float, struct IntegralTag> & buffer,
	int gridDensity, // Number of cells
	float amplitude)
{
	assert((buffer.Size.width % gridDensity) == 0);
	assert((buffer.Size.height % gridDensity) == 0);

	// Create grid (#edges = #cells + 1)
	IntegralRectSize const gridSize = IntegralRectSize(
		gridDensity + 1,
		gridDensity + 1);
	int const cellWidth = buffer.Size.width / gridDensity;
	assert((buffer.Size.width % cellWidth) == 0);
	int const cellHeight = buffer.Size.height / gridDensity;
	assert((buffer.Size.height % cellHeight) == 0);
	auto grid = MakePerlinVectorGrid(gridSize);

	// Make grid repeatable
	for (int x = 0; x < gridSize.width - 1; ++x)
		grid[{x, gridSize.height - 1}] = grid[{x, 0}];
	for (int y = 0; y < gridSize.height - 1; ++y)
		grid[{gridSize.width - 1, y}] = grid[{0, y}];
	grid[{gridSize.width - 1, gridSize.height - 1}] = grid[{0, 0}];

	// Create corner metadata
	vec2i const cornerVectors[4] = {
		{0, 0},
		{cellWidth, 0},
		{0, cellHeight},
		{cellWidth, cellHeight}
	};

	// Precalc scaling factor, which also ensures that dot product
	// results are between -1 and 1
	float const scalingFactor = amplitude / std::sqrtf(static_cast<float>(cellWidth * cellWidth + cellHeight * cellHeight));

	//
	// Visit all points in the abstract cell
	//

	float dy = 0.0f;
	for (int cy = 0; cy < cellHeight; ++cy, dy += 1.0f / static_cast<float>(cellHeight))
	{
		float const dy2 = SmoothStep(0.0f, 1.0f, dy);

		float dx = 0.0f;
		for (int cx = 0; cx < cellWidth; ++cx, dx += 1.0f / static_cast<float>(cellWidth))
		{
			float const dx2 = SmoothStep(0.0f, 1.0f, dx);

			// Calculate the four offset vectors
			vec2f offsetVectors[4];
			for (int corner = 0; corner < 4; ++corner)
			{
				offsetVectors[corner] = vec2f(
					static_cast<float>(cx - cornerVectors[corner].x),
					static_cast<float>(cy - cornerVectors[corner].y));
			}

			//
			// Visit all cells
			//

			vec2f const * gridPtr = grid.Data.get();
			float * bufferPtr = &(buffer[{cx, cy}]);

			for (int cellTopY = 0; cellTopY < gridDensity; ++cellTopY)
			{
				float * bufferPtrRow = bufferPtr;
				for (int cellLeftX = 0; cellLeftX < gridDensity; ++cellLeftX)
				{
					// Calc dot products of grid vectors with offset vectors for each corner
					float const dotProduct0 = offsetVectors[0].dot(*(gridPtr));
					float const dotProduct1 = offsetVectors[1].dot(*(gridPtr + 1));
					float const dotProduct2 = offsetVectors[2].dot(*(gridPtr + gridDensity + 1));
					float const dotProduct3 = offsetVectors[3].dot(*(gridPtr + 1 + gridDensity + 1));

					// Interpolate dot products at the candidate pos
					float const nTop = dotProduct0 * (1.0f - dx2) + dotProduct1 * dx2;
					float const nBottom = dotProduct2 * (1.0f - dx2) + dotProduct3 * dx2;
					float const n = nTop * (1.0f - dy2) + nBottom * dy2;

					// Store value
					*(bufferPtrRow) += n * scalingFactor;

					++gridPtr;
					bufferPtrRow += cellWidth;
				}

				++gridPtr; // Skip extra cell
				bufferPtr += buffer.Size.width * cellHeight; // Restart writing into next cell's row
			}
		}
	}
}