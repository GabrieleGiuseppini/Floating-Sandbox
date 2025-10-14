/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2025-10-13
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

	static void CalculateEdges(
		RgbaImageData const & source,
		std::vector<int> & leftX,
		std::vector<int> & rightX,
		std::vector<int> & topY,
		std::vector<int> & bottomY);

	static float CalculateWasteOnLeftEdge(
		std::vector<int> const & leftX,
		int offset,
		int structureMeshSize,
		int textureSize);

	static float CalculateWasteOnRightEdge(
		std::vector<int> const & rightX,
		int offset,
		int structureMeshSize,
		int textureSize);

private:

	static std::pair<int, int> CalculateOptimalOffsets(
		std::vector<int> const & leftX,
		std::vector<int> const & rightX,
		int minLeftX,
		int maxRightX,
		int structureMeshSize,
		int textureSize);
};

}