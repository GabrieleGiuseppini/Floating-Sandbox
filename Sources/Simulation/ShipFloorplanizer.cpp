/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2024-05-07
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipFloorplanizer.h"

#include "SimulationParameters.h"

#include <Core/GameChronometer.h>
#include <Core/Log.h>

#include <cassert>
#include <chrono>

ShipFactoryFloorPlan ShipFloorplanizer::BuildFloorplan(
	ShipFactoryPointIndexMatrix const & pointIndexMatrix,
	std::vector<ShipFactoryPoint> const & pointInfos2,
	IndexRemap const & pointIndexRemap,
	std::vector<ShipFactorySpring> const & springInfos2) const
{
	auto const startTime = GameChronometer::now();

	//
	// 1. Build list of springs that we do not want to use as floors;
	//    we do so by detecting specific vertex patterns in 3x3 blocks
	//

	SpringExclusionSet springExclusionSet;

	// Process all 3x3 blocks - including the 1-wide "borders"
	VertexBlock vertexBlock;
	for (int y = 0; y < pointIndexMatrix.height - 2; ++y)
	{
		for (int x = 0; x < pointIndexMatrix.width - 2; ++x)
		{
			// Build block
			for (int yb = 0; yb < 3; ++yb)
			{
				for (int xb = 0; xb < 3; ++xb)
				{
					ElementIndex vertexIndex = NoneElementIndex;

					auto const pointIndex1 = pointIndexMatrix[{x + xb, y + yb}];
					if (pointIndex1.has_value())
					{
						auto const pointIndex2 = pointIndexRemap.OldToNew(*pointIndex1);
						if (pointInfos2[pointIndex2].StructuralMtl.IsHull)
						{
							vertexIndex = pointIndex2;
						}
					}

					vertexBlock[xb][yb] = vertexIndex;
				}
			}

			// Process block
			ProcessVertexBlock(
				vertexBlock,
				springExclusionSet);
		}
	}

	//
	// 2. Build floorplan with All and ONLY "hull" springs which:
	//	- Are directly derived from structure, and
	//	- Are on the side of a triangle, and
	//	- Are not in the exclusione set
	//

	ShipFactoryFloorPlan floorPlan;
	floorPlan.reserve(springInfos2.size());

	for (size_t s = 0; s < springInfos2.size(); ++s)
	{
		auto const & springInfo2 = springInfos2[s];

		// Make sure it's viable as a floor and, if it's a non-external edge, it's not in the exclusion list
		if (IsSpringViableForFloor(springInfo2, pointInfos2)
			&& (springInfo2.Triangles.size() == 1 || springExclusionSet.count({ springInfo2.PointAIndex, springInfo2.PointBIndex}) == 0))
		{
			//
			// Take this spring
			//

			assert(pointInfos2[springInfo2.PointAIndex].DefinitionCoordinates.has_value());
			auto const pointADefinitionCoordinates = *(pointInfos2[springInfo2.PointAIndex].DefinitionCoordinates);

			assert(pointInfos2[springInfo2.PointBIndex].DefinitionCoordinates.has_value());
			auto const pointBDefinitionCoordinates = *(pointInfos2[springInfo2.PointBIndex].DefinitionCoordinates);

			NpcFloorGeometryType floorGeometry;
			if (pointADefinitionCoordinates.x == pointBDefinitionCoordinates.x)
			{
				// Vertical
				assert(std::abs(pointADefinitionCoordinates.y - pointBDefinitionCoordinates.y) == 1);
				floorGeometry = NpcFloorGeometryType::Depth1V;
			}
			else if (pointADefinitionCoordinates.y == pointBDefinitionCoordinates.y)
			{
				// Horizontal
				assert(std::abs(pointADefinitionCoordinates.x - pointBDefinitionCoordinates.x) == 1);
				floorGeometry = NpcFloorGeometryType::Depth1H;
			}
			else if (
				(pointADefinitionCoordinates.x < pointBDefinitionCoordinates.x)
				== (pointADefinitionCoordinates.y < pointBDefinitionCoordinates.y)
				)
			{
				// Diagonal 1
				assert(
					((pointADefinitionCoordinates.x - pointBDefinitionCoordinates.x) == 1
						&& (pointADefinitionCoordinates.y - pointBDefinitionCoordinates.y) == 1)
					||
					((pointADefinitionCoordinates.x - pointBDefinitionCoordinates.x) == -1
						&& (pointADefinitionCoordinates.y - pointBDefinitionCoordinates.y) == -1));
				floorGeometry = NpcFloorGeometryType::Depth2S1;
			}
			else
			{
				// Diagonal 2
				assert(
					((pointADefinitionCoordinates.x - pointBDefinitionCoordinates.x) == 1
						&& (pointADefinitionCoordinates.y - pointBDefinitionCoordinates.y) == -1)
					||
					((pointADefinitionCoordinates.x - pointBDefinitionCoordinates.x) == -1
						&& (pointADefinitionCoordinates.y - pointBDefinitionCoordinates.y) == 1));
				floorGeometry = NpcFloorGeometryType::Depth2S2;
			}

			auto const [_, isInserted] = floorPlan.try_emplace(
				{ springInfo2.PointAIndex, springInfo2.PointBIndex },
				NpcFloorKindType::DefaultFloor,
				floorGeometry,
				static_cast<ElementIndex>(s));

			assert(isInserted);
			(void)isInserted;
		}
	}

	LogMessage("ShipFloorplanizer: completed floorplan: floorTiles=", floorPlan.size(),
		" time=", std::chrono::duration_cast<std::chrono::microseconds>(GameChronometer::now() - startTime).count(), "us");

	return floorPlan;
}

bool ShipFloorplanizer::IsSpringViableForFloor(
	ShipFactorySpring const & springInfo2,
	std::vector<ShipFactoryPoint> const & pointInfos2) const
{
	return
		pointInfos2[springInfo2.PointAIndex].DefinitionCoordinates.has_value() // Is point derived directly from structure?
		&& pointInfos2[springInfo2.PointAIndex].StructuralMtl.IsHull // Is point hull?
		&& pointInfos2[springInfo2.PointBIndex].DefinitionCoordinates.has_value() // Is point derived directly from structure?
		&& pointInfos2[springInfo2.PointBIndex].StructuralMtl.IsHull // Is point hull?
		&& springInfo2.Triangles.size() > 0 // Is it an edge of a triangle?
		;
}

void ShipFloorplanizer::ProcessVertexBlock(
	VertexBlock & vertexBlock,
	/*out*/ SpringExclusionSet & springExclusionSet) const
{
	// 1. All rotations of symmetry 1

	for (int i = 0; i < 4; ++i)
	{
		ProcessVertexBlockPatterns(
			vertexBlock,
			springExclusionSet);

		Rotate90CW(vertexBlock);

	}

	// 2. All rotations of symmetry 2

	FlipV(vertexBlock);

	for (int i = 0; ; ++i)
	{
		ProcessVertexBlockPatterns(
			vertexBlock,
			springExclusionSet);

		// Save one rotation
		if (i == 3)
		{
			break;
		}

		Rotate90CW(vertexBlock);
	}
}

void ShipFloorplanizer::ProcessVertexBlockPatterns(
	VertexBlock const & vertexBlock,
	/*out*/ SpringExclusionSet & springExclusionSet) const
{
	//
	// Check for a set of specific patterns; once one is found,
	// exclude specific springs (which might not even exist)
	//

	//
	// Pattern 1: "under a stair" (_\): take care of redundant /
	//
	//   *?o
	//   o*?
	//   ***
	//

	if (vertexBlock[0][0] != NoneElementIndex && vertexBlock[1][0] != NoneElementIndex && vertexBlock[2][0] != NoneElementIndex
		&& vertexBlock[0][1] == NoneElementIndex && vertexBlock[1][1] != NoneElementIndex
		&& vertexBlock[0][2] != NoneElementIndex && vertexBlock[2][2] == NoneElementIndex)
	{
		springExclusionSet.insert({ vertexBlock[0][0] , vertexBlock[1][1] });
	}

	//
	// Pattern 2: "under a stair" (_\): take care of redundant |
	//
	//   *oo
	//   o*?
	//   ***
	//

	if (vertexBlock[0][0] != NoneElementIndex && vertexBlock[1][0] != NoneElementIndex && vertexBlock[2][0] != NoneElementIndex
		&& vertexBlock[0][1] == NoneElementIndex && vertexBlock[1][1] != NoneElementIndex
		&& vertexBlock[0][2] != NoneElementIndex && vertexBlock[1][2] == NoneElementIndex && vertexBlock[2][2] == NoneElementIndex)
	{
		springExclusionSet.insert({ vertexBlock[1][0] , vertexBlock[1][1] });
	}

	////// Questionable: seems superseded and removes useful
	//////
	////// Pattern 3: "wall-on-floor" (|-): take care of redundant \ and /
	//////
	//////  o?o
	//////  o*o
	//////  ***
	//////

	////if (vertexBlock[0][0] != NoneElementIndex && vertexBlock[1][0] != NoneElementIndex && vertexBlock[2][0] != NoneElementIndex
	////	&& vertexBlock[0][1] == NoneElementIndex && vertexBlock[1][1] != NoneElementIndex && vertexBlock[2][1] == NoneElementIndex
	////	&& vertexBlock[0][2] == NoneElementIndex && vertexBlock[2][2] == NoneElementIndex)
	////{
	////	springExclusionSet.insert({ vertexBlock[0][0] , vertexBlock[1][1] });
	////	springExclusionSet.insert({ vertexBlock[2][0] , vertexBlock[1][1] });
	////}

	//
	// Pattern 4: "corner" (|_): take care of redundant \
	//
	//  *o?
	//  *o?
	//  ***
	//

	if (vertexBlock[0][0] != NoneElementIndex && vertexBlock[1][0] != NoneElementIndex && vertexBlock[2][0] != NoneElementIndex
		&& vertexBlock[0][1] != NoneElementIndex && vertexBlock[1][1] == NoneElementIndex
		&& vertexBlock[0][2] != NoneElementIndex && vertexBlock[1][2] == NoneElementIndex)
	{
		springExclusionSet.insert({ vertexBlock[0][1] , vertexBlock[1][0] });
	}

	////// Questionable: was for sealed_triangles_test, but removes useful diagonal for video_mesh
	//////
	////// Pattern 5: "floor-meets-high-wall" (_||): take care of redundant /
	//////
	//////  o**
	//////  o**
	//////  ***
	//////

	////if (vertexBlock[0][0] != NoneElementIndex && vertexBlock[1][0] != NoneElementIndex && vertexBlock[2][0] != NoneElementIndex
	////	&& vertexBlock[0][1] == NoneElementIndex && vertexBlock[1][1] != NoneElementIndex && vertexBlock[2][1] != NoneElementIndex
	////	&& vertexBlock[0][2] == NoneElementIndex && vertexBlock[1][2] != NoneElementIndex && vertexBlock[2][2] != NoneElementIndex)
	////{
	////	springExclusionSet.insert({ vertexBlock[0][0] , vertexBlock[1][1] });
	////}

	//
	// Pattern 6: "stair at angle" (_\|): take care of redundant /| and /_
	//
	//   *o*
	//   o**
	//   ***
	//

	if (vertexBlock[0][0] != NoneElementIndex && vertexBlock[1][0] != NoneElementIndex && vertexBlock[2][0] != NoneElementIndex
		&& vertexBlock[0][1] == NoneElementIndex && vertexBlock[1][1] != NoneElementIndex && vertexBlock[2][1] != NoneElementIndex
		&& vertexBlock[0][2] != NoneElementIndex && vertexBlock[1][2] == NoneElementIndex && vertexBlock[2][2] != NoneElementIndex)
	{
		springExclusionSet.insert({ vertexBlock[0][0] , vertexBlock[1][1] });
		springExclusionSet.insert({ vertexBlock[1][1] , vertexBlock[1][0] });
		springExclusionSet.insert({ vertexBlock[1][1] , vertexBlock[2][2] });
		springExclusionSet.insert({ vertexBlock[1][1] , vertexBlock[2][1] });
	}
}

void ShipFloorplanizer::Rotate90CW(VertexBlock & vertexBlock) const
{
	auto const tmp1 = vertexBlock[0][0];
	vertexBlock[0][0] = vertexBlock[2][0];
	vertexBlock[2][0] = vertexBlock[2][2];
	vertexBlock[2][2] = vertexBlock[0][2];
	vertexBlock[0][2] = tmp1;

	auto const tmp2 = vertexBlock[1][0];
	vertexBlock[1][0] = vertexBlock[2][1];
	vertexBlock[2][1] = vertexBlock[1][2];
	vertexBlock[1][2] = vertexBlock[0][1];
	vertexBlock[0][1] = tmp2;
}

void ShipFloorplanizer::FlipV(VertexBlock & vertexBlock) const
{
	std::swap(vertexBlock[0][0], vertexBlock[0][2]);
	std::swap(vertexBlock[1][0], vertexBlock[1][2]);
	std::swap(vertexBlock[2][0], vertexBlock[2][2]);
}
