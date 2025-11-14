/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2025-11-13
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/ImageData.h>

#include <utility>
#include <vector>

namespace ShipBuilder {

class TextureAlignmentOptimizer_TODO final
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

	static inline float CalculateWasteOnLeftEdge(
		int leftX,
		int offset,
		int structureMeshSize,
		int textureSize);

	static inline void BlitColumn(
		RgbaImageData const & source,
		RgbaImageData & target,
		int x,
		int ySourceBottom,
		int ySourceTop,
		int yTargetBottom,
		int yTargetTop);
};

}