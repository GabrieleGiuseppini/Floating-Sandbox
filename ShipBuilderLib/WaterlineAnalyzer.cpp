/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "WaterlineAnalyzer.h"

#include <GameCore/Log.h>

#include <cassert>

namespace ShipBuilder {

WaterlineAnalyzer::WaterlineAnalyzer(Model const & model)
    : mModel(model)
    , mCurrentState(StateType::CalculateStaticResults)
{
}

bool WaterlineAnalyzer::Update()
{
    vec2f constexpr Vertical = vec2f(0.0f, -1.0f); // Vertical down

    switch (mCurrentState)
    {
        case StateType::CalculateStaticResults:
        {
            mStaticResults = CalculateStaticResults();

            assert(mStaticResults.has_value());

            if (mStaticResults->TotalMass == 0.0f)
            {
                // No particles, we're done
                mCurrentState = StateType::Completed;
                return true;
            }
            else
            {
                // Start search

                // Initialize level search

                mLevelSearchDirection = Vertical;

                std::tie(mLevelSearchLowest, mLevelSearchHighest) = CalculateLevelSearchLimits(
                    mStaticResults->CenterOfMass,
                    mLevelSearchDirection);

                mLevelSearchCurrent = 0.0f;

                // Transition state
                mCurrentState = StateType::FindLevel;

                return false;
            }
        }

        case StateType::FindLevel:
        {
            assert(mStaticResults.has_value());

            // Calculate waterline center - along <center of mass -> direction> vector, at current level
            vec2f const waterlineCenter =
                mStaticResults->CenterOfMass
                + mLevelSearchDirection * mLevelSearchCurrent;

            // Store this waterline
            mWaterline.emplace(
                waterlineCenter,
                mLevelSearchDirection);

            // Calculate buoyancy at this waterline
            std::tie(mTotalBuoyantForce, mCenterOfBuoyancy) = CalculateBuoyancy(*mWaterline);

            // Calculate next level

            if (*mTotalBuoyantForce > mStaticResults->TotalMass)
            {
                // Floating too much => it's too submersed;
                // this level is thus the new highest
                mLevelSearchHighest = mLevelSearchCurrent;
            }
            else
            {
                // Floating too little => needs to be more submersed;
                // this level is thus the new lowest
                mLevelSearchLowest = mLevelSearchCurrent;
            }

            assert(mLevelSearchLowest >= mLevelSearchHighest);
            float const newLevelSearchCurrent = mLevelSearchHighest + (mLevelSearchLowest - mLevelSearchHighest) / 2.0f;

            // TODO: check if close to a limit? Or may be this is taken care of by tolerance check below? Test w/floating object and w/submarine

            // Check if we haven't moved much from previous
            float constexpr LevelChangeTolerance = 1.0f;
            if (std::abs(newLevelSearchCurrent - mLevelSearchCurrent) < LevelChangeTolerance)
            {
                //
                // We have found the level
                //

                // Calculate CoM->CoB direction
                vec2f const mbDirection = (*mCenterOfBuoyancy - mStaticResults->CenterOfMass).normalise();

                // Check if "vertical enough"
                //float constexpr VerticalTolerance = 0.002f;
                float constexpr VerticalTolerance = 0.0001f;
                LogMessage("TODOTEST: mbDirection=", mbDirection.toString(), " searchDir=", mLevelSearchDirection.toString(), " alignment=", std::abs(mbDirection.dot(mLevelSearchDirection)));
                if (std::abs(mbDirection.dot(mLevelSearchDirection)) >= 1.0f - VerticalTolerance)
                {
                    //
                    // We're done
                    //

                    // Transition state
                    mCurrentState = StateType::Completed;

                    return true;
                }
                else
                {
                    //
                    // Calculate next search direction: apply torque to current direction
                    //

                    // alpha = mirror of angle between CoM->CoB and vertical, around vertical
                    float const cosAlpha = mbDirection.dot(mLevelSearchDirection);
                    float const sinAlpha = mbDirection.cross(mLevelSearchDirection);

                    // Rotate current search direction by alpha
                    mLevelSearchDirection = vec2f(
                        mLevelSearchDirection.x * cosAlpha - mLevelSearchDirection.y * sinAlpha,
                        mLevelSearchDirection.x * sinAlpha + mLevelSearchDirection.y * cosAlpha);

                    // Restart search from here
                    std::tie(mLevelSearchLowest, mLevelSearchHighest) = CalculateLevelSearchLimits(
                        mStaticResults->CenterOfMass,
                        mLevelSearchDirection);

                    mLevelSearchCurrent = 0.0f;

                    // Continue
                    return false;
                }
            }
            else
            {
                // Continue searching from here
                mLevelSearchCurrent = newLevelSearchCurrent;

                // Continue
                return false;
            }
        }

        case StateType::Completed:
        {
            assert(false);
            return true;
        }
    }

    assert(false);
    return false;
}

WaterlineAnalyzer::StaticResults WaterlineAnalyzer::CalculateStaticResults()
{
    float totalMass = 0.0f;
    vec2f centerOfMass = vec2f::zero();

    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;
    for (int y = 0; y < structuralLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayerBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            auto const * material = structuralLayerBuffer[coords].Material;
            if (material != nullptr)
            {
                totalMass += material->GetMass();
                centerOfMass += coords.ToFloat() * material->GetMass();
            }
        }
    }

    return StaticResults(
        totalMass,
        centerOfMass / (totalMass != 0.0f ? totalMass : 1.0f));
}

std::tuple<float, float> WaterlineAnalyzer::CalculateLevelSearchLimits(
    vec2f const & center,
    vec2f const & direction)
{
    // TODOTEST
    return std::make_tuple(
        center.y,
        -(static_cast<float>(mModel.GetShipSize().height) - center.y));
}

std::tuple<float, vec2f> WaterlineAnalyzer::CalculateBuoyancy(Waterline const & waterline)
{
    float constexpr WaterDensity = 1000.0f;

    float totalBuoyantForce = 0.0f;
    vec2f centerOfBuoyancySum = vec2f::zero();
    // TODOHACK: see below
    //size_t underwaterParticleCount = 0;

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
                float const alignment = (coordsF - waterline.Center).dot(waterline.WaterDirection);
                if (alignment >= 0.0f)
                {
                    // This point is on the "underwater" side of the center, along the direction

                    // TODOHACK: here we do the same as the simulator currently does, wrt "buoyancy volume fill".
                    // This needs to be removed once we have changed "buoyancy volume fill"
                    totalBuoyantForce += WaterDensity * material->BuoyancyVolumeFill;
                    centerOfBuoyancySum += coordsF * WaterDensity * material->BuoyancyVolumeFill;

                    // TODOHACK: THIS IS GOOD:
                    ////totalBuoyantForce += WaterDensity;
                    ////centerOfBuoyancySum += coordsF;
                    ////++underwaterParticleCount;
                }
            }
        }
    }

    return std::make_tuple(
        totalBuoyantForce,
        // TODOHACK: see above
        //centerOfBuoyancySum / (underwaterParticleCount > 0 ? static_cast<float>(underwaterParticleCount) : 1.0f));
        centerOfBuoyancySum / (totalBuoyantForce != 0.0f ? totalBuoyantForce : 1.0f));
}

}