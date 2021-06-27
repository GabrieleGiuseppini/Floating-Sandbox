/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuildTypes.h"

#include <GameCore/Matrix.h>
#include <GameCore/Vectors.h>

#include <cstdint>
#include <limits>
#include <vector>

class ShipStrengthRandomizer
{
public:

    ShipStrengthRandomizer();

    void RandomizeStrength(
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        vec2i const & pointIndexMatrixRegionOrigin,
        vec2i const & pointIndexMatrixRegionSize,
        std::vector<ShipBuildPoint> & pointInfos2,
        std::vector<ElementIndex> const & pointIndexRemap2,
        std::vector<ShipBuildSpring> const & springInfos2,
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        std::vector<ShipBuildFrontier> const & shipBuildFrontiers) const;

    //
    // Settings
    //

    float GetDensityAdjustment() const
    {
        return mDensityAdjustment;
    }

    void SetDensityAdjustment(float value)
    {
        mDensityAdjustment = value;
    }

    float GetRandomizationExtent() const
    {
        return mRandomizationExtent;
    }

    void SetRandomizationExtent(float value)
    {
        mRandomizationExtent = value;
    }

private:

    void RandomizeStrength_Perlin(std::vector<ShipBuildPoint> & pointInfos2) const;

    struct BatikDistance
    {
        float Distance;
        bool IsCrack;

        BatikDistance(float distance)
            : Distance(distance)
            , IsCrack(false)
        {
        }
    };

    using BatikDistanceMatrix = Matrix2<BatikDistance>;

    void RandomizeStrength_Batik(
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        vec2i const & pointIndexMatrixRegionOrigin,
        vec2i const & pointIndexMatrixRegionSize,
        std::vector<ShipBuildPoint> & pointInfos2,
        std::vector<ElementIndex> const & pointIndexRemap2,
        std::vector<ShipBuildSpring> const & springInfos2,
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        std::vector<ShipBuildFrontier> const & shipBuildFrontiers) const;

    template<typename TRandomEngine>
    void PropagateBatikCrack(
        vec2i const & startingPoint,
        BatikDistanceMatrix & distanceMatrix,
        TRandomEngine & randomEngine) const;

    void UpdateBatikDistances(BatikDistanceMatrix & distanceMatrix) const;

    template <typename TAcceptor>
    std::optional<Octant> FindClosestOctant(
        Octant startOctant,
        int maxOctantDivergence,
        TAcceptor const & acceptor) const;

private:

    //
    // Settings that we are the storage of
    //

    float mDensityAdjustment;
    float mRandomizationExtent;
};
