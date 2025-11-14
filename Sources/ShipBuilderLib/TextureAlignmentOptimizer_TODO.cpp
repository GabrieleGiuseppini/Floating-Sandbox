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
	// TODO: nop if pixelsPerShip is < ~4

	//
	// Calculate edges
	//
	//	* y=0 is at bottom, grows going up
	//

	std::vector<int> leftX(source.Size.height, source.Size.width);
	std::vector<int> rightX(source.Size.height, -1);
	std::vector<int> topY(source.Size.width, -1);
	std::vector<int> bottomY(source.Size.width, source.Size.height);

	CalculateEdges(source, leftX, rightX, topY, bottomY);

	//int const minX = *std::min_element(leftX.cbegin(), leftX.cend());
	//int const maxX = *std::max_element(rightX.cbegin(), rightX.cend());
	int const minY = *std::min_element(bottomY.cbegin(), bottomY.cend());
	int const maxY = *std::max_element(topY.cbegin(), topY.cend());

	//
	// Calculate segments
	//

	int constexpr MinStreakSize = 5;

	struct Segment
	{
		int StartIndex;
		int Length;
		int Value;
	};

	struct StreakSession
	{
		int StartIndex;
		int Value;
	};


	std::optional<StreakSession> currentStreak;

	// Bottom

	std::vector<Segment> bottomSegments;

	currentStreak.reset();

	for (int x = 0; x <= source.Size.width; ++x)
	{
		bool interruptsCurrentStreak = false;
		if (x == source.Size.width || bottomY[x] == source.Size.height)
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
				currentStreak.emplace(StreakSession{ x, newValue });
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
			if (streakLength >= MinStreakSize)
			{
				bottomSegments.emplace_back(Segment{
					currentStreak->StartIndex,
					streakLength,
					currentStreak->Value
					});
			}

			// Start new streak
			if (x == source.Size.width || bottomY[x] == source.Size.height)
			{
				currentStreak.reset();
			}
			else
			{
				currentStreak.emplace(StreakSession{ x, bottomY[x] });
			}
		}
	}

	LogMessage("Level 1 segments:");
	for (size_t s = 0; s < bottomSegments.size(); ++s)
	{
		if (s > 0 && bottomSegments[s].StartIndex != bottomSegments[s - 1].StartIndex + bottomSegments[s - 1].Length)
			LogMessage("");

		LogMessage("    @ ", bottomSegments[s].StartIndex, " len: ", bottomSegments[s].Length, "   ", bottomSegments[s].Value);
	}


	//
	// Find best offset wrt bottom segments
	//

	// Calculate overestimation of texture pixels per ship quad
	int const pixelsPerQuadH = static_cast<int>(std::ceilf(static_cast<float>(source.Size.height) / static_cast<float>(structureMeshSize.height)));

	int bestBottomOffset = 0;
	float minWaste = std::numeric_limits<float>::max();
	for (int bottomOffset = -pixelsPerQuadH; bottomOffset <= pixelsPerQuadH; ++bottomOffset)
	{
		if (minY + bottomOffset >= 0 && maxY + bottomOffset < source.Size.height)
		{
			float stepWaste = 0.0f;
			for (auto const & segment : bottomSegments)
			{
				float const segmentWaste = CalculateWasteOnLeftEdge(segment.Value, bottomOffset, structureMeshSize.height, source.Size.height);
				stepWaste += segmentWaste * static_cast<float>(segment.Length);
			}

			LogMessage("bottomOffset=", bottomOffset, " => waste=", stepWaste);

			if (stepWaste < minWaste)
			{
				bestBottomOffset = bottomOffset;
				minWaste = stepWaste;
			}
		}
	}

	LogMessage("-----");
	LogMessage("bestBottomOffset=", bestBottomOffset);

	//
	// Resample TODOTEST
	//

	//std::vector<int> newLeftX(leftX);
	//std::vector<int> newRightX(rightX);
	//std::vector<int> newTopY(topY);
	//std::vector<int> newBottomY(bottomY);

	// Bottom

	RgbaImageData newImage(source.Size, rgbaColor(rgbaColor::data_type_max, rgbaColor::data_type_max, rgbaColor::data_type_max, 0));

	size_t iCurrentSegment = 0;

	for (int x = 0; x < source.Size.width;)
	{
		if (bottomY[x] != source.Size.height)
		{
			assert(topY[x] >= 0);

			if (iCurrentSegment < bottomSegments.size() && x == bottomSegments[iCurrentSegment].StartIndex)
			{
				// Segment

				float const tx = static_cast<float>(bottomSegments[iCurrentSegment].Value);
				assert(bottomY[x] == bottomSegments[iCurrentSegment].Value);

				// s that covers pixel
				int const sx = static_cast<int>(std::floorf(tx / pixelsPerQuadH - 0.5f));
				assert(sx >= -1);

				// Now calculate t at the center of this ship quad - guaranteed to be to the left of or at tx
				float const tCenter = (static_cast<float>(sx) + 0.5f) * pixelsPerQuadH;
				assert(tx >= tCenter);

				// Don't tolerate waste larger than half quad
				int newBottomY = bottomY[x];
				LogMessage("waste=", tx - tCenter, " threshold=", pixelsPerQuadH / 2.0f);
				if ((tx - tCenter) > pixelsPerQuadH / 2.0f)
				{
					// Positionate at threshold - i.e. at beginning of sx+1
					newBottomY = static_cast<int>(std::ceilf(static_cast<float>(sx + 1) * pixelsPerQuadH));

					LogMessage("Larger than threshold: tx=", tx, " sx=", sx, "   ", bottomY[x], " -> ", newBottomY);

					LogMessage("  new waste=", static_cast<float>(newBottomY) - tCenter);
				}
				else
				{
					LogMessage("Smaller than threshold: tx=", tx, " sx=", sx);
				}

				for (int i = 0; i < bottomSegments[iCurrentSegment].Length; ++i)
				{
					int newTopY = topY[x + i];
					BlitColumn(source, newImage, x + i, bottomY[x], topY[x], newBottomY, newTopY);
				}

				x += bottomSegments[iCurrentSegment].Length;

				++iCurrentSegment;
			}
			else
			{
				// Column
				BlitColumn(source, newImage, x, bottomY[x], topY[x], bottomY[x], topY[x]);

				++x;
			}
		}
		else
		{
			assert(topY[x] == -1);

			++x;
		}
	}

	return newImage;
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

	if (sx < 0)
	{
		// leftX is to the left of the first possible tCenter, and thus the texture is clipped.
		// We penalize this situation as the worst
		return std::numeric_limits<float>::max() / 10.0f;
	}

	// Now calculate t at the center of this ship quad - guaranteed to be to the left of or at tx(o)
	float const tCenter = (static_cast<float>(sx) + 0.5f) * shipToTexture;
	assert(static_cast<float>(txo) >= tCenter);

	// Calculate waste
	return txo - tCenter;
}

void TextureAlignmentOptimizer_TODO::BlitColumn(
	RgbaImageData const & source,
	RgbaImageData & target,
	int x,
	int ySourceBottom,
	int ySourceTop,
	int yTargetBottom,
	int yTargetTop)
{
	float const deltaSource = static_cast<float>(ySourceTop - ySourceBottom);
	float const deltaTarget = static_cast<float>(yTargetTop - yTargetBottom);

	for (int y = yTargetBottom; y <= yTargetTop; ++y)
	{
		// Calculate source pixel
		float sourceYf = static_cast<float>(ySourceBottom) + static_cast<float>(y - yTargetBottom) / deltaTarget * deltaSource; // TODO: deltaTarget==0
		// Nearest
		int sourceY = static_cast<int>(std::roundf(sourceYf));

		target[{x, y}] = source[{x, sourceY}];
	}
}

}