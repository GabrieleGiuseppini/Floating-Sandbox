/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2025-10-13
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "TextureAlignmentOptimizer.h"

#include <Core/Log.h>

#include <algorithm>
#include <cassert>
#include <limits>

namespace ShipBuilder {

static int constexpr MaxTextureSize = 4096; // Upper limit to how much we want to go

RgbaImageData TextureAlignmentOptimizer::OptimizeAlignment(
	RgbaImageData const & source,
	ShipSpaceSize const & structureMeshSize)
{
	//
	// Calculate edges
	//
	//	* y=0 is at bottom, grows going up
	//

	std::vector<int> leftX;
	leftX.reserve(source.Size.height);
	std::vector<int> rightX;
	rightX.reserve(source.Size.height);
	std::vector<int> topY;
	topY.reserve(source.Size.width);
	std::vector<int> bottomY;
	bottomY.reserve(source.Size.width);

	CalculateEdges(source, leftX, rightX, topY, bottomY);

	assert(!leftX.empty());
	assert(!rightX.empty());
	assert(!topY.empty());
	assert(!bottomY.empty());

	int const minX = *std::min_element(leftX.cbegin(), leftX.cend());
	int const maxX = *std::max_element(rightX.cbegin(), rightX.cend());
	int const minY = *std::min_element(topY.cbegin(), topY.cend());
	int const maxY = *std::max_element(bottomY.cbegin(), bottomY.cend());

	LogMessage("X: ", minX, " -> ", maxX, "  Y: ", minY, " -> ", maxY);

	//
	// Optimize
	//
	// Find optimal combination of shift+stretch along each dimension at a time, which
	// minimizes waste
	//
	// Offsets' semantics:
	//	left > 0: pixels inserted to the left
	//	left < 0: pixels removed from the left
	//  right > 0: pixels removed from the right
	//  right < 0: pixels inserted from the right
	//

	// Horizontal
	std::pair<int, int> const bestHOffsets = CalculateOptimalOffsets(
		leftX,
		rightX,
		minX,
		maxX,
		structureMeshSize.width,
		source.Size.width);

	// Vertical
	std::pair<int, int> const bestVOffsets = CalculateOptimalOffsets(
		bottomY,
		topY,
		minY,
		maxY,
		structureMeshSize.height,
		source.Size.height);

	//
	// Calculate new texture size
	//

	ImageSize const newTextureSize = ImageSize(
		source.Size.width + bestHOffsets.first + bestHOffsets.second,
		source.Size.height + bestVOffsets.first + bestVOffsets.second);


	// TODOHERE
	auto const leftWaste = CalculateWasteOnLeftEdge(leftX, bestHOffsets.first, structureMeshSize.width, newTextureSize.width);
	auto const rightWaste = CalculateWasteOnRightEdge(rightX, bestHOffsets.first, structureMeshSize.width, newTextureSize.width);
	float const wasteH = leftWaste + rightWaste;
	auto const bottomWaste = CalculateWasteOnLeftEdge(bottomY, bestVOffsets.first, structureMeshSize.height, newTextureSize.height);
	auto const topWaste = CalculateWasteOnRightEdge(topY, bestVOffsets.first, structureMeshSize.height, newTextureSize.height);
	float const wasteV = bottomWaste + topWaste;
	LogMessage("Best offsets: H: ", bestHOffsets.first, ",", bestHOffsets.second, "  V: ", bestVOffsets.first, ",", bestVOffsets.second, "  WasteH: ", wasteH,
				" (", leftWaste, " + ", rightWaste, ") WasteV: ", wasteV, " (", bottomWaste, " + ", topWaste, ")");

	auto const leftWaste0 = CalculateWasteOnLeftEdge(leftX, 0, structureMeshSize.width, source.Size.width);
	auto const rightWaste0 = CalculateWasteOnRightEdge(rightX, 0, structureMeshSize.width, source.Size.width);
	float const wasteH0 = leftWaste0 + rightWaste0;
	auto const bottomWaste0 = CalculateWasteOnLeftEdge(bottomY, 0, structureMeshSize.height, source.Size.height);
	auto const topWaste0 = CalculateWasteOnRightEdge(topY, 0, structureMeshSize.height, source.Size.height);
	float const wasteV0 = bottomWaste0 + topWaste0;
	LogMessage("  WasteH0: ", wasteH0, " (", leftWaste0, " + ", rightWaste0, ") WasteV0: ", wasteV0, " (", bottomWaste0, " + ", topWaste0, ")");




	// TODOTEST
	return source.Clone();
}

void TextureAlignmentOptimizer::CalculateEdges(
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
			leftX.push_back(x);
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
			rightX.push_back(x);
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
			topY.push_back(y);
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
			bottomY.push_back(y);
		}
	}
}

float TextureAlignmentOptimizer::CalculateWasteOnLeftEdge(
	std::vector<int> const & leftX,
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

	float totalWaste = 0.0f;
	for (int tx : leftX)
	{
		int const txo = tx + offset;

		// s that covers this pixel
		int const sx = static_cast<int>(std::floorf(static_cast<float>(txo) / shipToTexture - 0.5f));
		assert(sx >= -1);
		if (sx < 0)
		{
			// leftX is to the left of tCenter now, and thus the texture is clipped.
			// We penalize this situation as the worst
			return std::numeric_limits<float>::max();
		}

		// Now calculate t at the center of this ship quad - guaranteed to be to the left of or at tx(o)
		float const tCenter = (static_cast<float>(sx) + 0.5f) * shipToTexture;
		assert(static_cast<float>(txo) >= tCenter);

		// Calculate waste: we consider pixel at tx(o) ending (to the left) at tx(o)
		totalWaste += (txo - tCenter);
	}

	return totalWaste;
}

float TextureAlignmentOptimizer::CalculateWasteOnRightEdge(
	std::vector<int> const & rightX,
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

	float totalWaste = 0.0f;
	for (int tx : rightX)
	{
		int const txo = tx + offset;

		// s that covers this pixel
		int const sx = static_cast<int>(std::floorf(static_cast<float>(txo) / shipToTexture - 0.5f)) + 1;
		assert(sx <= structureMeshSize);
		if (sx == structureMeshSize)
		{
			// rightX is to the right of tCenter now, and thus the texture is clipped.
			// We penalize this situation as the worst
			return std::numeric_limits<float>::max();
		}

		// Now calculate t at the center of this ship quad - guaranteed to be to the right of or at tx(o)
		float const tCenter = (static_cast<float>(sx) + 0.5f) * shipToTexture;
		assert(static_cast<float>(txo) <= tCenter);

		// Calculate waste: we consider pixel at tx(o) ending (to the right) at tx(o)+1,
		// i.e. we consider the pixel's width
		totalWaste += std::fabs(tCenter - (txo + 1));
	}

	return totalWaste;
}

std::pair<int, int> TextureAlignmentOptimizer::CalculateOptimalOffsets(
	std::vector<int> const & leftX,
	std::vector<int> const & rightX,
	int minLeftX,
	int maxRightX,
	int structureMeshSize,
	int textureSize)
{
	assert(minLeftX >= 0 && minLeftX <= textureSize - 1);
	assert(maxRightX >= 0 && maxRightX <= textureSize - 1);

	// Offsets' semantics:
	//	left > 0: pixels inserted to the left
	//	left < 0: pixels removed from the left
	//  right > 0: pixels inserted from the right
	//  right < 0: pixels removed from the right

	// Calculate overestimation of texture pixels per ship quad
	int const pixelsPerQuad = static_cast<int>(std::ceilf(static_cast<float>(textureSize) / static_cast<float>(structureMeshSize)));

	// Loop for offsets between -ppq (but constrained to not remove any pixels) and ppq finding minimum;
	// reason to constrain is to maintain similar size
	std::pair<int, int> bestOffsets{ 0, 0 };
	float minWaste = std::numeric_limits<float>::max();
	for (int leftOffset = -std::min(pixelsPerQuad, minLeftX); leftOffset <= pixelsPerQuad; ++leftOffset)
	{
		for (int rightOffset = -std::min(pixelsPerQuad, textureSize - maxRightX - 1); rightOffset <= pixelsPerQuad; ++rightOffset)
		{
			// Calculate new texture size
			int const newTextureSize = textureSize + leftOffset + rightOffset;
			if (newTextureSize <= MaxTextureSize)
			{
				auto const leftWaste = CalculateWasteOnLeftEdge(leftX, leftOffset, structureMeshSize, newTextureSize);
				auto const rightWaste = CalculateWasteOnRightEdge(rightX, leftOffset, structureMeshSize, newTextureSize);
				float const newWaste = leftWaste + rightWaste;

				LogMessage("@ ", leftOffset, ",", rightOffset, ": newWaste=", newWaste, " (", leftWaste, " + ", rightWaste, ") leftOffset=", leftOffset, " textureSize=", newTextureSize);

				if (newWaste < minWaste)
				{
					bestOffsets = { leftOffset, rightOffset };
					minWaste = newWaste;
				}
			}
		}
	}

	return bestOffsets;
}

}