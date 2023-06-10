/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipFactoryTypes.h"

#include <GameCore/IndexRemap.h>
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
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        vec2i const & pointIndexMatrixRegionOrigin,
        vec2i const & pointIndexMatrixRegionSize,
        std::vector<ShipFactoryPoint> & pointInfos2,
        IndexRemap const & pointIndexRemap,
        std::vector<ShipFactorySpring> const & springInfos2,
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        std::vector<ShipFactoryFrontier> const & shipFactoryFrontiers) const;

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

    void RandomizeStrength_Perlin(std::vector<ShipFactoryPoint> & pointInfos2) const;

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
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        vec2i const & pointIndexMatrixRegionOrigin,
        vec2i const & pointIndexMatrixRegionSize,
        std::vector<ShipFactoryPoint> & pointInfos2,
        IndexRemap const & pointIndexRemap,
        std::vector<ShipFactorySpring> const & springInfos2,
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        std::vector<ShipFactoryFrontier> const & shipFactoryFrontiers) const;

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
