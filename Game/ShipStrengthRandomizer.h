/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuildTypes.h"

#include <GameCore/Matrix.h>
#include <GameCore/Vectors.h>

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

    struct BatikPixel
    {
        float Distance;
        bool IsCrack;

        BatikPixel()
            : Distance(std::numeric_limits<float>::max())
            , IsCrack(false)
        {
        }
    };

    using BatikPixelMatrix = Matrix2<BatikPixel>;

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
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        vec2i const & pointIndexMatrixOffset,
        BatikPixelMatrix & pixelMatrix,
        TRandomEngine & randomEngine) const;

    void UpdateBatikDistances(BatikPixelMatrix & pixelMatrix) const;

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
