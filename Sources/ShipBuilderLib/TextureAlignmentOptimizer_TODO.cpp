/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2025-11-13
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "TextureAlignmentOptimizer_TODO.h"

#include "WorkbenchState.h"

#include <Core/Log.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <optional>

namespace ShipBuilder {

RgbaImageData TextureAlignmentOptimizer_TODO::OptimizeAlignment(
	RgbaImageData const & source,
	ShipSpaceSize const & structureMeshSize)
{
	//
	// Calculate edges
	//
	//	* y=0 is at bottom, grows going up
	//

	std::vector<int> leftX(source.Size.height, -1); // -1 is marker for "not existing"
	std::vector<int> rightX(source.Size.height, -1);
	std::vector<int> topY(source.Size.width, -1);
	std::vector<int> bottomY(source.Size.width, -1);

	CalculateEdges(source, leftX, rightX, topY, bottomY);

	//
	// TODOTEST
	//

	int constexpr MinStreakSize = 5;

	std::vector<int> newLeftX(leftX);
	std::vector<int> newRightX(rightX);
	std::vector<int> newTopY(topY);
	std::vector<int> newBottomY(bottomY);

	struct StreakSession
	{
		int StartIndex;
		int Value;
	};

	std::optional<StreakSession> currentStreak;

	// Bottom

	currentStreak.reset();

	for (int x = 0; x <= source.Size.width; ++x)
	{
		bool interruptsCurrentStreak = false;
		if (x == source.Size.width || bottomY[x] == -1)
		{
			// Interrupt streak if we have one
			interruptsCurrentStreak = currentStreak.has_value();
		}
		else
		{
			int const newValue = bottomY[x];
			if (!currentStreak.has_value())
			{
				// Start streak
				currentStreak.emplace(StreakSession{x, newValue});
			}
			else if (newValue != currentStreak->Value)
			{
				interruptsCurrentStreak = true;
			}
		}

		if (interruptsCurrentStreak)
		{
			assert(currentStreak.has_value());

			int const streakLength = x - currentStreak->StartIndex;
			assert(streakLength > 0);
			if (streakLength > MinStreakSize)
			{
				LogMessage("Streak: ", streakLength, " X ", currentStreak->Value, " @ ", currentStreak->StartIndex, " -> ", x);
			}

			// Start new streak
			if (x == source.Size.width || bottomY[x] == -1)
			{
				currentStreak.reset();
			}
			else
			{
				currentStreak.emplace(StreakSession{ x, bottomY[x] });
			}
		}
	}

	// TODOTEST
	LogMessage("-------------------------------------");

	int const shipToTextureH = static_cast<int>(std::ceilf(static_cast<float>(source.Size.height) / static_cast<float>(structureMeshSize.height)));
	LogMessage("shipToTextureH=", shipToTextureH);
	for (int y = 0; y < shipToTextureH * 2; ++y)
	{
		float const waste = CalculateWasteOnLeftEdge(y, 0, structureMeshSize.height, source.Size.height);
		LogMessage("y=", y, ": ", waste);
	}


	// TODOTEST
	(void)structureMeshSize;
	return source.Clone();
}

void TextureAlignmentOptimizer_TODO::CalculateEdges(
	RgbaImageData const & source,
	std::vector<int> & leftX,
	std::vector<int> & rightX,
	std::vector<int> & topY,
	std::vector<int> & bottomY)
{
	float constexpr AlphaThreshold = 0.5f;

	// Horizontal (from bottom to up)

	for (int y = 0; y < source.Size.height; ++y)
	{
		int x = 0;
		for (; x < source.Size.width; ++x)
		{
			bool const isPixelFull = source[{x, y}].a > AlphaThreshold;
			if (isPixelFull)
			{
				break;
			}
		}

		if (x < source.Size.width)
		{
			leftX[y] = x;
		}

		x = source.Size.width - 1;
		for (; x >= 0; --x)
		{
			bool const isPixelFull = source[{x, y}].a > AlphaThreshold;
			if (isPixelFull)
			{
				break;
			}
		}

		if (x >= 0)
		{
			rightX[y] = x;
		}
	}

	// Vertical (from left to right)

	for (int x = 0; x < source.Size.width; ++x)
	{
		int y = source.Size.height - 1;
		for (; y >= 0; --y)
		{
			bool const isPixelFull = source[{x, y}].a > AlphaThreshold;
			if (isPixelFull)
			{
				break;
			}
		}

		if (y >= 0)
		{
			topY[x] = y;
		}

		y = 0;
		for (; y < source.Size.height; ++y)
		{
			bool const isPixelFull = source[{x, y}].a > AlphaThreshold;
			if (isPixelFull)
			{
				break;
			}
		}

		if (y < source.Size.height)
		{
			bottomY[x] = y;
		}
	}
}

float TextureAlignmentOptimizer_TODO::CalculateWasteOnLeftEdge(
	int leftX,
	int offset,
	int structureMeshSize,
	int textureSize)
{
	//
	// Calculate number of pixels wasted by a structure that completely covers edge
	//
	// The pixel at coordinate t is covered by the line between ship coordinate s(t) and s(t+1).
	// The formula for s(t) is the "texturization" one, i.e. s = (t - o/2) / o, where o is the number of texture
	// pixels in one ship quad.
	//

	float const shipToTexture = static_cast<float>(textureSize) / static_cast<float>(structureMeshSize);

	int const txo = leftX + offset;

	// s that covers this pixel
	int const sx = static_cast<int>(std::floorf(static_cast<float>(txo) / shipToTexture - 0.5f));
	assert(sx >= -1);

	// Now calculate t at the center of this ship quad - guaranteed to be to the left of or at tx(o)
	float const tCenter = (static_cast<float>(sx) + 0.5f) * shipToTexture;
	assert(static_cast<float>(txo) >= tCenter);

	// Calculate waste
	return txo - tCenter;
}

}