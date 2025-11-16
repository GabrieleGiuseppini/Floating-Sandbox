/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2025-11-15
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/ImageData.h>

#include <utility>
#include <vector>

namespace ShipBuilder {

class TextureAlignmentOptimizer final
{
public:

	static RgbaImageData OptimizeAlignment(
		RgbaImageData const & source,
		ShipSpaceSize const & structureMeshSize);

private:

	static void CalculateEdges(
		RgbaImageData const & source,
		std::vector<int> & leftX,
		std::vector<int> & rightX,
		std::vector<int> & topY,
		std::vector<int> & bottomY);

	struct Segment
	{
		int StartIndex;
		int Length;
		int Value;
	};

	static std::vector<Segment> CalculateSegments(
		std::vector<int> const & xValues,
		int emptyXValue,
		int minSegmentLength);

	static std::pair<int, int> CalculateOptimalOffsets(
		std::vector<Segment> const & leftXSegments,
		std::vector<Segment> const & rightXSegments,
		int minLeftX,
		int maxRightX,
		int structureMeshSize,
		int textureSize);

	static inline float CalculateWasteOnLeftEdge(
		std::vector<Segment> const & leftXSegments,
		int offset,
		int structureMeshSize,
		int textureSize);

	static inline float CalculateWasteOnLeftEdge(
		int leftX,
		int offset,
		int structureMeshSize,
		int textureSize);

	static inline float CalculateWasteOnRightEdge(
		std::vector<Segment> const & rightXSegments,
		int offset,
		int structureMeshSize,
		int textureSize);

	static inline float CalculateWasteOnRightEdge(
		int rightX,
		int offset,
		int structureMeshSize,
		int textureSize);
};

}