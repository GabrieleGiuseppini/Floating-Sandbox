/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "WaterlineAnalyzer.h"

#include <Core/GameMath.h>

#include <cassert>
#include <limits>

namespace ShipBuilder {

float constexpr WaterDensity = 1000.0f;
vec2f constexpr Vertical2f = vec2f(0.0f, -1.0f); // Vertical down

WaterlineAnalyzer::WaterlineAnalyzer(IModelObservable const & model)
    : mModel(model)
    , mModelMacroProperties(mModel.GetModelMacroProperties())
{
}

bool WaterlineAnalyzer::Update()
{
    float constexpr LevelSearchStride = 1.5f;
    float constexpr LevelSearchChangeTolerance = 0.5f;
    float constexpr TorqueToDirectionRotationAngleFactor = 0.05f;
    float constexpr DirectionRotationAngleStrideMax = 0.2f;

    if (mModelMacroProperties.MassParticleCount == 0)
    {
        // No particles, we're done
        return true;
    }

    assert(mModelMacroProperties.CenterOfMass.has_value());

    if (!mTotalBuoyantForceWhenFullySubmerged.has_value())
    {
        //
        // Do a first calculation of buoyancy assuming the ship is fully submersed
        //

        vec2f centerOfBuoyancyWhenSubmersed;
        std::tie(mTotalBuoyantForceWhenFullySubmerged, centerOfBuoyancyWhenSubmersed) = CalculateBuoyancyWithWaterline(
            vec2f(static_cast<float>(mModel.GetShipSize().width) / 2.0f, static_cast<float>(mModel.GetShipSize().height)),
            Vertical2f);

        //
        // Initialize search
        //

        mDirectionSearchCWAngleMax = Pi<float>;
        mDirectionSearchCWAngleMin = -Pi<float>;

        if (*mTotalBuoyantForceWhenFullySubmerged < mModelMacroProperties.TotalMass * 0.98f)
        {
            // This ship is sinking; we can jumpstart search by:
            // - Initializing level search to "fully submerged"
            // - Initializing direction as perpendicular to CoM->CoB

            mDirectionSearchCurrent = (centerOfBuoyancyWhenSubmersed - *(mModelMacroProperties.CenterOfMass)).normalise();

            std::tie(mLevelSearchLowest, mLevelSearchHighest) = CalculateLevelSearchLimits(
                *(mModelMacroProperties.CenterOfMass),
                mDirectionSearchCurrent);

            mLevelSearchCurrent = mLevelSearchHighest;
        }
        else
        {
            // This ship is (in all likelihood) floating; we then start search normally, i.e.:
            // - Initializing level search at zero, i.e. at center of mass (likely place_
            // - Initializing direction as fully vertical

            mDirectionSearchCurrent = Vertical2f;

            std::tie(mLevelSearchLowest, mLevelSearchHighest) = CalculateLevelSearchLimits(
                *(mModelMacroProperties.CenterOfMass),
                mDirectionSearchCurrent);

            mLevelSearchCurrent = 0.0f;
        }

        // Continue
        return false;
    }

    //
    // Calculate buoyancy at current waterline
    //

    // Calculate waterline center - along <center of mass -> direction> vector, at current level
    vec2f const waterlineCenter =
        *(mModelMacroProperties.CenterOfMass)
        + mDirectionSearchCurrent * mLevelSearchCurrent;

    // Store this waterline
    mWaterline.emplace(
        waterlineCenter,
        mDirectionSearchCurrent);

    // Calculate buoyancy at this waterline
    vec2f newCenterOfBuoyancy;
    std::tie(mTotalBuoyantForce, newCenterOfBuoyancy) = CalculateBuoyancyWithWaterline(
        mWaterline->Center,
        mWaterline->WaterDirection);

    assert(mTotalBuoyantForce.has_value());

    //
    // Calculate next level
    //

    assert(mLevelSearchHighest <= mLevelSearchCurrent && mLevelSearchCurrent <= mLevelSearchLowest);

    float newLevelSearchCurrent;
    if (*mTotalBuoyantForce > mModelMacroProperties.TotalMass)
    {
        // Floating too much => it's too submersed;
        // this level is thus the new highest (i.e. limit at the top)
        mLevelSearchHighest = mLevelSearchCurrent;

        // Move search down
        newLevelSearchCurrent = mLevelSearchCurrent + LevelSearchStride;
        if (newLevelSearchCurrent >= mLevelSearchLowest)
        {
            // Too much, bisect available room
            newLevelSearchCurrent = mLevelSearchCurrent + (mLevelSearchLowest - mLevelSearchCurrent) / 2.0f;
        }
    }
    else
    {
        // Floating too little => needs to be more submersed;
        // this level is thus the new lowest (i.e. limit at the bottom)
        mLevelSearchLowest = mLevelSearchCurrent;

        // Move search up
        newLevelSearchCurrent = mLevelSearchCurrent - LevelSearchStride;
        if (newLevelSearchCurrent <= mLevelSearchHighest)
        {
            // Too much, bisect available room
            newLevelSearchCurrent = mLevelSearchCurrent - (mLevelSearchCurrent - mLevelSearchHighest) / 2.0f;
        }
    }

    // Check if we haven't moved much from previous
    if (std::abs(newLevelSearchCurrent - mLevelSearchCurrent) < LevelSearchChangeTolerance)
    {
        //
        // We have found the level
        //

        // Finalize center of buoyancy
        if (mTotalBuoyantForce != 0.0f)
        {
            mCenterOfBuoyancy = newCenterOfBuoyancy;
        }
        else
        {
            mCenterOfBuoyancy.reset();
        }

        //
        // Calculate next search direction
        //

        // Calculate CW angle of search direction wrt real vertical
        // (positive when search direction is CW wrt vertical)
        float const directionVerticalAlphaCW = Vertical2f.angleCw(mDirectionSearchCurrent);

        // Calculate "torque" (massless) of weight/buoyancy on CoM->CoB direction
        float const torque = mDirectionSearchCurrent.cross(newCenterOfBuoyancy - *(mModelMacroProperties.CenterOfMass));

        // Calculate (delta-) rotation we want to rotate direction for
        float directionRotationCW;
        if (torque <= 0.0f)
        {
            // Torque rotates ship CW, hence generates a CCW rotation of the direction - i.e. a negative CW rotation
            directionRotationCW = std::max(
                torque * TorqueToDirectionRotationAngleFactor, // Negative torque is ship CW rotation, hence a CCW rotation of the direction
                -DirectionRotationAngleStrideMax);

            // Current angle is the new maximum
            mDirectionSearchCWAngleMax = directionVerticalAlphaCW;

            // Check if we'd overshoot limits after this rotation
            if (directionVerticalAlphaCW + directionRotationCW <= mDirectionSearchCWAngleMin)
            {
                // Too much, bisect available room
                directionRotationCW = (mDirectionSearchCWAngleMin - directionVerticalAlphaCW) / 2.0f;
            }
        }
        else
        {
            // Torque rotates ship CCW, hence generates a CW rotation of the direction - i.e. a positive CW rotation
            directionRotationCW = std::min(
                torque * TorqueToDirectionRotationAngleFactor, // Negative torque is ship CW rotation, hence a CCW rotation of the direction
                DirectionRotationAngleStrideMax);

            // Current angle is the new minimum
            mDirectionSearchCWAngleMin = directionVerticalAlphaCW;

            // Check if we'd overshoot limits after this rotation
            if (directionVerticalAlphaCW + directionRotationCW >= mDirectionSearchCWAngleMax)
            {
                // Too much, bisect available room
                directionRotationCW = (mDirectionSearchCWAngleMax - directionVerticalAlphaCW) / 2.0f;
            }
        }

        // Check if too small a rotation
        float constexpr RotationTolerance = 0.001f;
        if (std::abs(directionRotationCW) <= RotationTolerance)
        {
            //
            // We're done
            //

            return true;
        }

        // Rotate current search direction
        mDirectionSearchCurrent = mDirectionSearchCurrent.rotate(-directionRotationCW);

        //
        // Restart search from here
        //

        // Calculate new limits
        std::tie(mLevelSearchLowest, mLevelSearchHighest) = CalculateLevelSearchLimits(
            *(mModelMacroProperties.CenterOfMass),
            mDirectionSearchCurrent);

        // Clamp direction, in case rotation has restricted legal range for current level
        mLevelSearchCurrent = Clamp(mLevelSearchCurrent, mLevelSearchHighest, mLevelSearchLowest);

        assert(mLevelSearchHighest <= mLevelSearchCurrent && mLevelSearchCurrent <= mLevelSearchLowest);

        // Continue
        return false;
    }
    else
    {
        // Continue searching from here
        mLevelSearchCurrent = newLevelSearchCurrent;

        // Continue
        return false;
    }
}

std::tuple<float, float> WaterlineAnalyzer::CalculateLevelSearchLimits(
    vec2f const & center,
    vec2f const & direction)
{
    float const canvasWidth = static_cast<float>(mModel.GetShipSize().width);
    float const canvasHeight = static_cast<float>(mModel.GetShipSize().height);

    std::array<vec2f, 4> corners = {
        vec2f{0.0f, 0.0f},
        vec2f{0.0f, canvasHeight},
        vec2f{canvasWidth, 0.0f},
        vec2f{canvasWidth, canvasHeight}
    };

    float tLowest = std::numeric_limits<float>::lowest(); // This will have the highest numerical value (positive "below")
    float tHighest = std::numeric_limits<float>::max(); // This will have the lowest numerical value (negative "above")
    for (size_t i = 0; i < corners.size(); ++i)
    {
        // Calculate t along <center, direction> vector V such that the vector
        // from this corner to V * t is perpendicular to V

        float const t = direction.dot(corners[i] - center);

        tLowest = std::max(tLowest, t);
        tHighest = std::min(tHighest, t);
    }

    return std::make_tuple(tLowest, tHighest);
}

std::tuple<float, vec2f> WaterlineAnalyzer::CalculateBuoyancyWithWaterline(
    vec2f const & waterlineCenter,
    vec2f const & waterlineDirection)
{
    float totalBuoyantForce = 0.0f;
    vec2f centerOfBuoyancySum = vec2f::zero();

    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;
    for (int y = 0; y < structuralLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayerBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            auto const * material = structuralLayerBuffer[coords].Material;
            if (material != nullptr)
            {
                // Check alignment with direction
                //
                // Note: here we take a particle's bottom-left corner as the point for which
                // we check its direction
                auto const coordsF = coords.ToFloat();
                float const alignment = (coordsF - waterlineCenter).dot(waterlineDirection);
                if (alignment >= 0.0f)
                {
                    // This point is on the "underwater" side of the center, along the direction

                    // Note: here we do the same as the simulator currently does wrt "buoyancy volume fill"
                    totalBuoyantForce += WaterDensity * material->BuoyancyVolumeFill;
                    centerOfBuoyancySum += coordsF * WaterDensity * material->BuoyancyVolumeFill;
                }
            }
        }
    }

    return std::make_tuple(
        totalBuoyantForce,
        centerOfBuoyancySum / (totalBuoyantForce != 0.0f ? totalBuoyantForce : 1.0f));
}

}