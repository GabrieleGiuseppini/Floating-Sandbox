/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipStrengthRandomizer.h"

#include <GameCore/Log.h>

#include <random>

static vec2i const OctantDirections[8] = {
    {  1,  0 },  // 0: E
    {  1, -1 },  // 1: SE
    {  0, -1 },  // 2: S
    { -1, -1 },  // 3: SW
    { -1,  0 },  // 4: W
    { -1,  1 },  // 5: NW
    {  0,  1 },  // 6: N
    {  1,  1 }   // 7: NE
};

ShipStrengthRandomizer::ShipStrengthRandomizer()
    // Settings defaults
    : mDensityAdjustment(1.0f)
    , mRandomizationExtent(0.4f)
{
}

void ShipStrengthRandomizer::RandomizeStrength(
    ShipBuildPointIndexMatrix const & pointIndexMatrix,
    vec2i const & pointIndexMatrixRegionOrigin,
    vec2i const & pointIndexMatrixRegionSize,
    std::vector<ShipBuildPoint> & pointInfos2,
    std::vector<ElementIndex> const & pointIndexRemap2,
    std::vector<ShipBuildSpring> const & springInfos2,
    std::vector<ShipBuildTriangle> const & triangleInfos1,
    std::vector<ShipBuildFrontier> const & shipBuildFrontiers) const
{
    RandomizeStrength_Batik(
        pointIndexMatrix,
        pointIndexMatrixRegionOrigin,
        pointIndexMatrixRegionSize,
        pointInfos2,
        pointIndexRemap2,
        springInfos2,
        triangleInfos1,
        shipBuildFrontiers);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void ShipStrengthRandomizer::RandomizeStrength_Perlin(std::vector<ShipBuildPoint> & pointInfos2) const
{
    if (mDensityAdjustment == 0.0f
        || mRandomizationExtent == 0.0f)
    {
        // Nothing to do
        return;
    }

    //
    // Basic Perlin noise generation
    //
    // Deterministic randomness
    //

    float constexpr CellWidth = 4.0f;

    auto const gradientVectorAt = [](float x, float y) -> vec2f // Always positive
    {
        float const arg = (1.0f + std::sin(x * (x * 12.9898f + y * 78.233f))) * 43758.5453f;
        float const random = arg - std::floor(arg);
        return vec2f(random, random);
    };

    for (auto & point : pointInfos2)
    {
        // We don't want to randomize the strength of ropes
        if (!point.IsRope)
        {
            // Coordinates of point in grid space
            vec2f const gridPos(
                static_cast<float>(point.Position.x) / CellWidth,
                static_cast<float>(point.Position.y) / CellWidth);

            // Coordinates of four cell corners
            float const x0 = floor(gridPos.x);
            float const x1 = x0 + 1.0f;
            float const y0 = floor(gridPos.y);
            float const y1 = y0 + 1.0f;

            // Offset vectors from corners
            vec2f const off00 = gridPos - vec2f(x0, y0);
            vec2f const off10 = gridPos - vec2f(x1, y0);
            vec2f const off01 = gridPos - vec2f(x0, y1);
            vec2f const off11 = gridPos - vec2f(x1, y1);

            // Gradient vectors at four corners
            vec2f const gv00 = gradientVectorAt(x0, y0);
            vec2f const gv10 = gradientVectorAt(x1, y0);
            vec2f const gv01 = gradientVectorAt(x0, y1);
            vec2f const gv11 = gradientVectorAt(x1, y1);

            // Dot products at each corner
            float const dp00 = off00.dot(gv00);
            float const dp10 = off10.dot(gv10);
            float const dp01 = off01.dot(gv01);
            float const dp11 = off11.dot(gv11);

            // Interpolate four dot products at this point (using a bilinear)
            float const interpx1 = Mix(dp00, dp10, off00.x);
            float const interpx2 = Mix(dp01, dp11, off00.x);
            float const perlin = Mix(interpx1, interpx2, off00.y);

            // Randomize strength
            point.Strength *=
                (1.0f - mRandomizationExtent)
                + mRandomizationExtent * std::sqrt(std::abs(perlin));
        }
    }
}

void ShipStrengthRandomizer::RandomizeStrength_Batik(
    ShipBuildPointIndexMatrix const & pointIndexMatrix,
    vec2i const & pointIndexMatrixRegionOrigin,
    vec2i const & pointIndexMatrixRegionSize,
    std::vector<ShipBuildPoint> & pointInfos2,
    std::vector<ElementIndex> const & pointIndexRemap2,
    std::vector<ShipBuildSpring> const & springInfos2,
    std::vector<ShipBuildTriangle> const & triangleInfos1,
    std::vector<ShipBuildFrontier> const & shipBuildFrontiers) const
{
    if (mDensityAdjustment == 0.0f
        || mRandomizationExtent == 0.0f)
    {
        // Nothing to do
        return;
    }

    //
    // Adapted from https://www.researchgate.net/publication/221523196_Rendering_cracks_in_Batik
    //
    // Main features:
    //  - A crack should pass through a point that is at (locally) maximal distance from any earlier crack,
    //    since there the stress is (locally) maximal;
    //  - A crack should propagate as fast as possible to the nearest feature (i.e.earlier crack or border of the wax)
    //

    auto const startTime = std::chrono::steady_clock::now();

    // Setup deterministic randomness

    std::seed_seq seq({ 1, 242, 19730528 });
    std::ranlux48_base randomEngine(seq);

    std::uniform_int_distribution<size_t> pointChoiceDistribution(0, triangleInfos1.size() * 3);

    //
    // Create distance map
    //

    BatikPixelMatrix pixelMatrix(
        pointIndexMatrixRegionSize.x,
        pointIndexMatrixRegionSize.y);

    //
    // Initialize distance map with distances from frontiers
    //

    for (ShipBuildFrontier const & frontier : shipBuildFrontiers)
    {
        for (ElementIndex springIndex2 : frontier.EdgeIndices2)
        {
            auto const pointAIndex2 = pointIndexRemap2[springInfos2[springIndex2].PointAIndex1];
            auto const & coordsA = pointInfos2[pointAIndex2].OriginalDefinitionCoordinates;
            if (coordsA.has_value())
            {
                pixelMatrix[*coordsA + vec2i(1, 1) - pointIndexMatrixRegionOrigin].Distance = 0.0f;
            }

            auto const pointBIndex2 = pointIndexRemap2[springInfos2[springIndex2].PointBIndex1];
            auto const & coordsB = pointInfos2[pointBIndex2].OriginalDefinitionCoordinates;
            if (coordsB.has_value())
            {
                pixelMatrix[*coordsB + vec2i(1, 1) - pointIndexMatrixRegionOrigin].Distance = 0.0f;
            }
        }
    }

    //
    // Generate cracks
    //

    // Choose number of cracks
    int const numberOfCracks = static_cast<int>(
        static_cast<float>(std::max(pointIndexMatrixRegionSize.x, pointIndexMatrixRegionSize.y))
        / 4.0f
        * mDensityAdjustment);

    for (int iCrack = 0; iCrack < numberOfCracks; ++iCrack)
    {
        //
        // Update distances
        //

        UpdateBatikDistances(pixelMatrix);

        //
        // Choose a starting point among all triangle vertices
        //

        auto const randomDraw = pointChoiceDistribution(randomEngine);
        ElementIndex const startingPointIndex2 = pointIndexRemap2[triangleInfos1[randomDraw / 3].PointIndices1[randomDraw % 3]];
        if (!pointInfos2[startingPointIndex2].OriginalDefinitionCoordinates.has_value())
            continue;

        vec2i startingPointCoords = *pointInfos2[startingPointIndex2].OriginalDefinitionCoordinates + vec2i(1, 1) - pointIndexMatrixRegionOrigin;

        // Navigate in distance map to find local maximum
        while (true)
        {
            std::optional<vec2i> bestPointCoords;
            float maxDistance = pixelMatrix[startingPointCoords].Distance;

            for (int octant = 0; octant < 8; ++octant)
            {
                vec2i const candidateCoords = startingPointCoords + OctantDirections[octant];
                if (pointIndexMatrix[candidateCoords + pointIndexMatrixRegionOrigin].has_value()
                    && pixelMatrix[candidateCoords].Distance > maxDistance)
                {
                    maxDistance = pixelMatrix[candidateCoords].Distance;
                    bestPointCoords = candidateCoords;
                }
            }

            if (!bestPointCoords.has_value())
            {
                // We're done
                break;
            }

            // Advance
            startingPointCoords = *bestPointCoords;
        }

        //
        // Find initial direction == direction of steepest descent of D
        //

        std::optional<Octant> bestNextPointOctant;
        float maxDelta = std::numeric_limits<float>::lowest();
        for (Octant octant = 0; octant < 8; ++octant)
        {
            vec2i const candidateCoords = startingPointCoords + OctantDirections[octant];
            if (pointIndexMatrix[candidateCoords + pointIndexMatrixRegionOrigin].has_value())
            {
                float const delta = pixelMatrix[startingPointCoords].Distance - pixelMatrix[candidateCoords].Distance;
                if (delta >= maxDelta)
                {
                    maxDelta = delta;
                    bestNextPointOctant = octant;
                }
            }
        }

        if (bestNextPointOctant.has_value())
        {
            //
            // Propagate crack along this direction
            //

            PropagateBatikCrack(
                startingPointCoords + OctantDirections[*bestNextPointOctant],
                pointIndexMatrix,
                pointIndexMatrixRegionOrigin,
                pixelMatrix,
                randomEngine);

            //
            // Find (closest point to) opposite direction
            //

            auto const oppositeOctant = FindClosestOctant(
                *bestNextPointOctant + 4,
                2,
                [&](Octant candidateOctant)
                {
                    vec2i const candidateCoords = startingPointCoords + OctantDirections[candidateOctant];
                    return pointIndexMatrix[candidateCoords + pointIndexMatrixRegionOrigin].has_value();
                });

            if (oppositeOctant.has_value())
            {
                PropagateBatikCrack(
                    startingPointCoords + OctantDirections[*oppositeOctant],
                    pointIndexMatrix,
                    pointIndexMatrixRegionOrigin,
                    pixelMatrix,
                    randomEngine);
            }
        }

        // Set crack at starting point
        pixelMatrix[startingPointCoords].Distance = 0.0f;
        pixelMatrix[startingPointCoords].IsCrack = true;

    }

    //
    // Randomize strengths
    //

    for (int x = 0; x < pixelMatrix.Width; ++x)
    {
        for (int y = 0; y < pixelMatrix.Height; ++y)
        {
            vec2i const pointCoords(x, y);

            if (auto const & pointIndex1 = pointIndexMatrix[pointCoords + pointIndexMatrixRegionOrigin];
                pixelMatrix[pointCoords].IsCrack
                && pointIndex1.has_value()
                && !pointInfos2[pointIndexRemap2[*pointIndex1]].ConnectedTriangles1.empty())
            {
                pointInfos2[pointIndexRemap2[*pointIndex1]].Strength *= (1.0f - mRandomizationExtent);
            }
        }
    }

    // TODOHERE

    ///////////////////////////////////////////////////////////////////////////
    // TODOTEST
    /*
    float maxDistance = 0.0f;
    for (int x = 0; x < pixelMatrix.Width; ++x)
    {
        for (int y = 0; y < pixelMatrix.Height; ++y)
        {
            if (pixelMatrix[{x, y}].Distance > maxDistance)
            {
                maxDistance = pixelMatrix[{x, y}].Distance;
            }
        }
    }

    LogMessage("TODOTEST: MaxDistance=", maxDistance);

    for (int x = 0; x < pixelMatrix.Width; ++x)
    {
        for (int y = 0; y < pixelMatrix.Height; ++y)
        {
            vec2i const pointCoors(x, y);
            auto const & idx1 = pointIndexMatrix[pointCoors + pointIndexMatrixRegionOrigin];
            if (idx1.has_value())
            {
                pointInfos2[pointIndexRemap2[*idx1]].Strength = pixelMatrix[{x, y}].Distance / maxDistance;
            }
        }
    }
    */

    LogMessage("ShipStrengthRandomizer: completed randomization:",
        " numberOfCracks=", numberOfCracks,
        " time=", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count(), "us");
}

template<typename TRandomEngine>
void ShipStrengthRandomizer::PropagateBatikCrack(
    vec2i const & startingPoint,
    ShipBuildPointIndexMatrix const & pointIndexMatrix,
    vec2i const & pointIndexMatrixRegionOrigin,
    BatikPixelMatrix & pixelMatrix,
    TRandomEngine & randomEngine) const
{
    auto const directionPerturbationDistribution = std::uniform_int_distribution(-1, 1);

    //
    // Propagate crack along descent derivative of distance, until a point
    // at distance zero (border or other crack) is reached
    //

    std::vector<vec2i> crackPointCoords;

    for (vec2i p = startingPoint; ;)
    {
        crackPointCoords.emplace_back(p);

        //
        // Check whether we're done
        //

        if (pixelMatrix[p].Distance == 0.0f)
        {
            // Reached border or another crack, done
            break;
        }

        //
        // Find direction of steepest descent
        //

        std::optional<Octant> bestNextPointOctant;
        float maxDelta = std::numeric_limits<float>::lowest();
        for (Octant octant = 0; octant < 8; ++octant)
        {
            vec2i const candidateCoords = p + OctantDirections[octant];
            if (pointIndexMatrix[candidateCoords + pointIndexMatrixRegionOrigin].has_value())
            {
                float const delta = pixelMatrix[p].Distance - pixelMatrix[candidateCoords].Distance;
                if (delta >= maxDelta)
                {
                    maxDelta = delta;
                    bestNextPointOctant = octant;
                }
            }
        }

        if (!bestNextPointOctant.has_value())
        {
            // No more continuing
            break;
        }

        //
        // Randomize the direction
        //

        bestNextPointOctant = FindClosestOctant(
            *bestNextPointOctant + directionPerturbationDistribution(randomEngine),
            2,
            [&](Octant candidateOctant)
        {
            vec2i const candidateCoords = p + OctantDirections[candidateOctant];
            return pointIndexMatrix[candidateCoords + pointIndexMatrixRegionOrigin].has_value();
        });

        //
        // Follow this point
        //

        p = p + OctantDirections[*bestNextPointOctant];
    }

    for (auto const & p : crackPointCoords)
    {
        pixelMatrix[p].Distance = 0.0f;
        pixelMatrix[p].IsCrack = true;
    }
}

void ShipStrengthRandomizer::UpdateBatikDistances(BatikPixelMatrix & pixelMatrix) const
{
    //
    // Jain's algorithm (1989, Fundamentals of Digital Image Processing, Chapter 2)
    //

    // Top-Left -> Bottom-Right
    for (int x = 0; x < pixelMatrix.Width; ++x)
    {
        for (int y = pixelMatrix.Height - 1; y >= 0; --y)
        {
            vec2i const idx(x, y);

            // Upper left half of 8-neighborhood of (x, y)
            for (int t = 4; t <= 7; ++t)
            {
                vec2i const nidx = idx + OctantDirections[t];
                if (nidx.IsInRect(pixelMatrix)
                    && pixelMatrix[nidx].Distance + 1.0f < pixelMatrix[idx].Distance)
                {
                    pixelMatrix[idx].Distance = pixelMatrix[nidx].Distance + 1.0f;
                }
            }
        }
    }

    // Bottom-Right -> Top-Left
    for (int x = pixelMatrix.Width - 1; x >= 0; --x)
    {
        for (int y = 0; y < pixelMatrix.Height; ++y)
        {
            vec2i const idx(x, y);

            // Lower right half of 8-neighborhood of (x, y)
            for (int t = 0; t <= 3; ++t)
            {
                vec2i const nidx = idx + OctantDirections[t];
                if (nidx.IsInRect(pixelMatrix)
                    && pixelMatrix[nidx].Distance + 1.0f < pixelMatrix[idx].Distance)
                {
                    pixelMatrix[idx].Distance = pixelMatrix[nidx].Distance + 1.0f;
                }
            }
        }
    }
}

template <typename TAcceptor>
std::optional<Octant> ShipStrengthRandomizer::FindClosestOctant(
    Octant startOctant,
    int maxOctantDivergence,
    TAcceptor const & acceptor) const
{
    if (startOctant < 0)
        startOctant += 8;
    startOctant = startOctant % 8;

    if (acceptor(startOctant))
    {
        return startOctant;
    }

    for (int deltaOctant = 1; deltaOctant <= maxOctantDivergence; ++deltaOctant)
    {
        Octant octant = (startOctant + deltaOctant) % 8;
        if (acceptor(octant))
        {
            return octant;
        }

        octant = startOctant - deltaOctant;
        if (octant < 0)
            octant += 8;
        octant = octant % 8;
        if (acceptor(octant))
        {
            return octant;
        }
    }

    return std::nullopt;
}