/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "WaterlineAnalyzer.h"

#include <cassert>

namespace ShipBuilder {

WaterlineAnalyzer::WaterlineAnalyzer(Model const & model)
    : mModel(model)
    , mCurrentState(StateType::CalculateStaticResults)
{
}

bool WaterlineAnalyzer::Update()
{
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

                mLevelSearchDirection = vec2f(0.0f, -1.0f); // Vertical down

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

            // Check if close enough to floating
            // TODO: it's integral!!! Won't ever be subject to tolerance this easily
            float constexpr BuoyantForceTolerance = 500.0f; // Magic number
            if (std::abs(*mTotalBuoyantForce - mStaticResults->TotalMass) < BuoyantForceTolerance)
            {
                // We have found the level

                // TODOHERE: and so we now need to find a direction;
                // If new one is near the previous, we're done

                // TODOTEST: simulating done now
            }
            else
            {
                // Need to change level

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

                // Check if any is close to a limit now
                // TODO: really needed? Can't piggyback on "other" tolerance check?
                // TODOHERE

                assert(mLevelSearchLowest >= mLevelSearchHighest);
                mLevelSearchCurrent = mLevelSearchHighest + (mLevelSearchLowest - mLevelSearchHighest) / 2.0f;

                // Continue
                return false;
            }

            // TODOTEST: simulating done now

            //
            // Done
            //

            // Transition state
            mCurrentState = StateType::Completed;

            return true;
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
        static_cast<float>(mModel.GetShipSize().height) - center.y);
}

std::tuple<float, vec2f> WaterlineAnalyzer::CalculateBuoyancy(Waterline const & waterline)
{
    float constexpr WaterDensity = 1000.0f;

    float totalBuoyantForce = 0.0f;
    vec2f centerOfBuoyancySum = vec2f::zero();
    size_t underwaterParticleCount = 0;

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

                    totalBuoyantForce += WaterDensity;
                    centerOfBuoyancySum += coordsF;
                    ++underwaterParticleCount;
                }
            }
        }
    }

    return std::make_tuple(
        totalBuoyantForce,
        centerOfBuoyancySum / (underwaterParticleCount > 0 ? static_cast<float>(underwaterParticleCount) : 1.0f));
}

}