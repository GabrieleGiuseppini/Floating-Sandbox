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

            //
            // Calculate buoyancy
            //

            //auto [buoyantForce, centerOfBuoyancy] = CalculateBuoyancy(
            std::tie(mTotalBuoyantForce, mCenterOfBuoyancy) = CalculateBuoyancy(
                mStaticResults->CenterOfMass,
                mLevelSearchDirection,
                mLevelSearchCurrent);

            // TODOHERE: check if close enough to mass; if so, we've found a level,
            // and so we now need to find a direction;
            //      - if new one is near the previous, we're done
            // if not, we halve, then check if close to a limit, if so we're done (see if may use previous for this one)
            //

            // TODOTEST: simulating done now

            //
            // Done
            //

            // Calculate center - along vector (center of mass -> direction) at current y
            vec2f const waterlineCenter =
                mStaticResults->CenterOfMass
                + mLevelSearchDirection * mLevelSearchCurrent;

            mWaterline.emplace(
                waterlineCenter,
                mLevelSearchDirection);

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

std::tuple<float, vec2f> WaterlineAnalyzer::CalculateBuoyancy(
    vec2f const & center,
    vec2f const & direction,
    float level)
{
    float constexpr WaterDensity = 1000.0f;

    float totalBuoyantForce = 0.0f;
    vec2f centerOfBuoyancySum = vec2f::zero();
    size_t underwaterParticleCount = 0;

    vec2f const startBuoyancyPos = center + direction * level;

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
                float const alignment = (coordsF - startBuoyancyPos).dot(direction);
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