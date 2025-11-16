/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2025-11-15
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "TextureAlignmentOptimizer.h"

#include "WorkbenchState.h"

#include <Core/Log.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <optional>

namespace ShipBuilder {

RgbaImageData TextureAlignmentOptimizer::OptimizeAlignment(
	RgbaImageData const & source,
	ShipSpaceSize const & structureMeshSize)
{
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

	int const minX = *std::min_element(leftX.cbegin(), leftX.cend());
	int const maxX = *std::max_element(rightX.cbegin(), rightX.cend());
	int const minY = *std::min_element(bottomY.cbegin(), bottomY.cend());
	int const maxY = *std::max_element(topY.cbegin(), topY.cend());

	//
	// Calculate segments
	//

	// Calculate overestimation of texture pixels per ship quad, to use for min segment lengths
	int const minSegmentHLength = static_cast<int>(std::ceilf(static_cast<float>(source.Size.width) / static_cast<float>(structureMeshSize.width)));
	int const minSegmentVLength = static_cast<int>(std::ceilf(static_cast<float>(source.Size.height) / static_cast<float>(structureMeshSize.height)));

	LogMessage("TextureAlignmentOptimizer: pixelsPerQuadH=", minSegmentHLength, " pixelsPerQuadW=", minSegmentVLength);

	std::vector<Segment> leftSegments = CalculateSegments(leftX, source.Size.width, minSegmentVLength);
	std::vector<Segment> rightSegments = CalculateSegments(rightX, -1, minSegmentVLength);
	std::vector<Segment> bottomSegments = CalculateSegments(bottomY, source.Size.height, minSegmentHLength);
	std::vector<Segment> topSegments = CalculateSegments(topY, -1, minSegmentHLength);

	//LogMessage("Left segments:");
	//for (auto const & s : leftSegments)
	//{
	//	LogMessage("    @ ", s.StartIndex, " len: ", s.Length, "   ", s.Value);
	//}

	//LogMessage("Right segments:");
	//for (auto const & s : rightSegments)
	//{
	//	LogMessage("    @ ", s.StartIndex, " len: ", s.Length, "   ", s.Value);
	//}

	//LogMessage("Bottom segments:");
	//for (auto const & s : bottomSegments)
	//{
	//	LogMessage("    @ ", s.StartIndex, " len: ", s.Length, "   ", s.Value);
	//}

	//LogMessage("Top segments:");
	//for (auto const & s : topSegments)
	//{
	//	LogMessage("    @ ", s.StartIndex, " len: ", s.Length, "   ", s.Value);
	//}

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
		leftSegments,
		rightSegments,
		minX,
		maxX,
		structureMeshSize.width,
		source.Size.width);

	LogMessage("TextureAlignmentOptimizer: bestHOffsets: ", std::get<0>(bestHOffsets), ", ", std::get<1>(bestHOffsets));

	// Vertical
	std::pair<int, int> const bestVOffsets = CalculateOptimalOffsets(
		bottomSegments,
		topSegments,
		minY,
		maxY,
		structureMeshSize.height,
		source.Size.height);

	LogMessage("TextureAlignmentOptimizer: bestVOffsets: ", std::get<0>(bestVOffsets), ", ", std::get<1>(bestVOffsets));

	//
	// Create new texture
	//

	ImageSize const newTextureSize = ImageSize(
		source.Size.width + bestHOffsets.first + bestHOffsets.second,
		source.Size.height + bestVOffsets.first + bestVOffsets.second);

	assert(newTextureSize.width >= 0);
	assert(source.Size.width > WorkbenchState::GetMaxTextureDimension() || newTextureSize.width <= WorkbenchState::GetMaxTextureDimension());
	assert(newTextureSize.height >= 0);
	assert(source.Size.height > WorkbenchState::GetMaxTextureDimension() || newTextureSize.height <= WorkbenchState::GetMaxTextureDimension());

	RgbaImageData newImage(newTextureSize, rgbaColor(rgbaColor::data_type_max, rgbaColor::data_type_max, rgbaColor::data_type_max, 0));

	// Blit source portion into it
	ImageCoordinates sourceOrigin(
		bestHOffsets.first < 0 ? -bestHOffsets.first : 0,
		bestVOffsets.first < 0 ? -bestVOffsets.first : 0);
	ImageCoordinates targetOrigin(
		bestHOffsets.first > 0 ? bestHOffsets.first : 0,
		bestVOffsets.first > 0 ? bestVOffsets.first : 0);
	newImage.BlitFromRegion(
		source,
		ImageRect(
			sourceOrigin,
			ImageSize(
				source.Size.width - sourceOrigin.x,
				source.Size.height - sourceOrigin.y)),
		targetOrigin);

	return newImage;
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

std::vector<TextureAlignmentOptimizer::Segment> TextureAlignmentOptimizer::CalculateSegments(
	std::vector<int> const & xValues,
	int emptyXValue,
	int minSegmentLength)
{
	struct SegmentSession
	{
		int StartIndex;
		int Value;
	};

	std::optional<SegmentSession> currentSegment;

	std::vector<Segment> segments;
	for (int x = 0; x <= xValues.size(); ++x)
	{
		bool interruptsCurrentSegment = false;
		if (x == xValues.size() || xValues[x] == emptyXValue)
		{
			// Interrupt segment if we have one
			interruptsCurrentSegment = currentSegment.has_value();
		}
		else
		{
			int const newValue = xValues[x];
			if (!currentSegment.has_value())
			{
				// Start segment
				currentSegment.emplace(SegmentSession{ x, newValue });
			}
			else if (newValue != currentSegment->Value)
			{
				interruptsCurrentSegment = true;
			}
		}

		if (interruptsCurrentSegment)
		{
			assert(currentSegment.has_value());

			int const segmentLength = x - currentSegment->StartIndex;
			assert(segmentLength > 0);
			if (segmentLength >= minSegmentLength)
			{
				segments.emplace_back(Segment{
					currentSegment->StartIndex,
					segmentLength,
					currentSegment->Value
					});
			}

			// Start new segment
			if (x == xValues.size() || xValues[x] == emptyXValue)
			{
				currentSegment.reset();
			}
			else
			{
				currentSegment.emplace(SegmentSession{ x, xValues[x] });
			}
		}
	}

	return segments;
}

std::pair<int, int> TextureAlignmentOptimizer::CalculateOptimalOffsets(
	std::vector<Segment> const & leftXSegments,
	std::vector<Segment> const & rightXSegments,
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

	// Calculate search radii - magic numbers to constrain stretching
	int const searchRadiusShift = pixelsPerQuad * 8;
	int const searchRadiusStretch = pixelsPerQuad * 2;

	// Loop for offsets between -searchRadius (but constrained to not remove any pixels) and searchRadius, finding minimum;
	// reason for limits is to maintain similar size as much as possible
	std::pair<int, int> bestOffsets{ 0, 0 };
	float minWaste = std::numeric_limits<float>::max();
	for (int leftOffset = -std::min(searchRadiusShift, minLeftX); leftOffset <= searchRadiusShift; ++leftOffset)
	{
		for (int rightOffset = -std::min(searchRadiusStretch, textureSize - maxRightX - 1); rightOffset <= searchRadiusStretch; ++rightOffset)
		{
			// Calculate new texture size
			int const newTextureSize = textureSize + leftOffset + rightOffset;
			if (newTextureSize <= WorkbenchState::GetMaxTextureDimension())
			{
				auto const leftWaste = CalculateWasteOnLeftEdge(leftXSegments, leftOffset, structureMeshSize, newTextureSize);
				auto const rightWaste = CalculateWasteOnRightEdge(rightXSegments, leftOffset, structureMeshSize, newTextureSize);

				if (leftOffset == 0 && rightOffset == 0)
					LogMessage("TextureAlignmentOptimizer: @L=", leftOffset, " R=", rightOffset, ": Waste: ", leftWaste, " ", rightWaste);

				float const newWaste = leftWaste + rightWaste;
				if (newWaste < minWaste
					|| (newWaste == minWaste && std::abs(leftOffset) <= std::abs(std::get<0>(bestOffsets)) && std::abs(rightOffset) <= std::abs(std::get<1>(bestOffsets))))
				{
					LogMessage("TextureAlignmentOptimizer: @L=", leftOffset, " R=", rightOffset, ": Waste: ", leftWaste, " ", rightWaste);

					bestOffsets = { leftOffset, rightOffset };
					minWaste = newWaste;
				}
			}
		}
	}

	return bestOffsets;
}

float TextureAlignmentOptimizer::CalculateWasteOnLeftEdge(
	std::vector<Segment> const & leftXSegments,
	int offset,
	int structureMeshSize,
	int textureSize)
{
	float waste = 0.0f;
	for (auto const & segment : leftXSegments)
	{
		waste += CalculateWasteOnLeftEdge(segment.Value, offset, structureMeshSize, textureSize) * static_cast<float>(segment.Length);
	}

	return waste;
}

float TextureAlignmentOptimizer::CalculateWasteOnLeftEdge(
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
	float const tCenter = std::roundf((static_cast<float>(sx) + 0.5f) * shipToTexture);
	assert(static_cast<float>(txo) >= tCenter);

	// Calculate waste
	return txo - tCenter;
}

float TextureAlignmentOptimizer::CalculateWasteOnRightEdge(
	std::vector<Segment> const & rightXSegments,
	int offset,
	int structureMeshSize,
	int textureSize)
{
	float waste = 0.0f;
	for (auto const & segment : rightXSegments)
	{
		waste += CalculateWasteOnRightEdge(segment.Value, offset, structureMeshSize, textureSize) * static_cast<float>(segment.Length);
	}

	return waste;
}

float TextureAlignmentOptimizer::CalculateWasteOnRightEdge(
	int rightX,
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

	int const txo = rightX + offset;

	// s that covers this pixel
	int const sx = static_cast<int>(std::floorf(static_cast<float>(txo) / shipToTexture - 0.5f)) + 1;
	assert(sx <= structureMeshSize);

	if (sx == structureMeshSize)
	{
		// rightX is to the right of tCenter now, and thus the texture is clipped.
		// We penalize this situation as the worst
		return std::numeric_limits<float>::max() / 10.0f;
	}

	// Now calculate t at the center of this ship quad - guaranteed to be to the left of or at tx(o)
	float const tCenter = std::roundf((static_cast<float>(sx) + 0.5f) * shipToTexture);
	assert(static_cast<float>(txo) <= tCenter);

	// Calculate waste: we consider pixel at tx(o) ending (to the right) at tx(o)+1,
	// i.e. we consider the pixel's width
	return std::fabs(tCenter - (txo + 1));
}

}