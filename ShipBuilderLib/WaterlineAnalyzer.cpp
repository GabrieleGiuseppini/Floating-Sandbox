/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "WaterlineAnalyzer.h"

#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

#include <cassert>
#include <limits>

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

                mDirectionSearchCurrent = Vertical;

                std::tie(mLevelSearchLowest, mLevelSearchHighest) = CalculateLevelSearchLimits(
                    mStaticResults->CenterOfMass,
                    mDirectionSearchCurrent);
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
                + mDirectionSearchCurrent * mLevelSearchCurrent;

            // Store this waterline
            mWaterline.emplace(
                waterlineCenter,
                mDirectionSearchCurrent);

            // Calculate buoyancy at this waterline
            vec2f newCenterOfBuoyancy;
            std::tie(mTotalBuoyantForce, newCenterOfBuoyancy) = CalculateBuoyancy(
                mWaterline->Center,
                mWaterline->WaterDirection);

            assert(mTotalBuoyantForce.has_value());

            //
            // Calculate next level
            //

            assert(mLevelSearchLowest >= mLevelSearchHighest);
            assert(mLevelSearchCurrent <= mLevelSearchLowest && mLevelSearchCurrent  >= mLevelSearchHighest);

            float constexpr LevelSearchStepSize = 2.0f; // TODO: make dependant on ship (canvas) size

            float newLevelSearchCurrent;

            if (*mTotalBuoyantForce > mStaticResults->TotalMass)
            {
                // Floating too much => it's too submersed;
                // this level is thus the new highest (i.e. limit at the top)
                mLevelSearchHighest = mLevelSearchCurrent;

                // Move search down
                newLevelSearchCurrent = mLevelSearchCurrent + LevelSearchStepSize;
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
                newLevelSearchCurrent = mLevelSearchCurrent - LevelSearchStepSize;
                if (newLevelSearchCurrent <= mLevelSearchHighest)
                {
                    // Too much, bisect available room
                    newLevelSearchCurrent = mLevelSearchCurrent - (mLevelSearchCurrent - mLevelSearchHighest) / 2.0f;
                }
            }

            // TODO: check if close to a limit? Or may be this is taken care of by tolerance check below? Test w/floating object and w/submarine

            // Check if we haven't moved much from previous
            float constexpr LevelChangeTolerance = 0.5f;
            if (std::abs(newLevelSearchCurrent - mLevelSearchCurrent) < LevelChangeTolerance)
            {
                //
                // We have found the level
                //

                // Finalize center of buoyancy
                mCenterOfBuoyancy = newCenterOfBuoyancy;

                // Calculate CoM->CoB direction
                vec2f const mb = (*mCenterOfBuoyancy - mStaticResults->CenterOfMass);
                vec2f const mbDirection = mb.normalise();

                //
                // Calculate "torque" of weight/buoyancy on CoM->CoB direction
                //

                float const torque = mDirectionSearchCurrent.cross(mb);

                LogMessage("TODOTEST: mb=", mb.toString(), " dir=", mbDirection.toString(), " torque=", torque);

                // Check if "vertical enough"
                float constexpr VerticalTolerance = 1.0f;
                if (std::abs(torque) <= VerticalTolerance)
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
                    // Calculate next search direction
                    //

                    // TODOTEST
                    // Torque-based, single step
                    {
                        // alpha = angle between CoM->CoB and "vertical"; positive when mbDirection
                        // is to the right (i.e. CW) of mLevelSearchDirection (when seen from CoM)
                        float const mbAlphaCW = mDirectionSearchCurrent.angleCw(mbDirection);

                        float alphaCcw;
                        float constexpr TorqueToAngleFactor = 0.0025f; // 0.005f was faster
                        float constexpr MaxAngle = 0.2f;
                        if (torque >= 0.0f)
                        {
                            // TODOTEST
                            //alphaCcw = -std::min(MaxAngle, torque * TorqueToAngleFactor);
                            alphaCcw = -torque * TorqueToAngleFactor;
                        }
                        else
                        {
                            // TODOTEST
                            //alphaCcw = -std::max(-MaxAngle, torque * TorqueToAngleFactor);
                            alphaCcw = -torque * TorqueToAngleFactor;
                        }

                        // Rotate current search direction
                        mDirectionSearchCurrent = mDirectionSearchCurrent.rotate(alphaCcw);

                        LogMessage("TODOTEST: alphaCW=", mbAlphaCW, " => rotation=", alphaCcw, " newDir=", mDirectionSearchCurrent.toString(), " newVerticalAlpha=", mDirectionSearchCurrent.angleCw(Vertical));

                        // TODOHERE: limit angle
                        ////if (torque >= 0.0f && mDirectionSearchCurrent.angleCw(Vertical) > mNegativeTorqueVerticalAngleCWMin)
                        ////{
                        ////    mDirectionSearchCurrent = Vertical.rotate(-mNegativeTorqueVerticalAngleCWMin);
                        ////}
                        ////else if (torque < 0.0f && mDirectionSearchCurrent.angleCw(Vertical) > mPositiveTorqueVerticalAngleCWMin)
                        ////{
                        ////    mDirectionSearchCurrent = Vertical.rotate(-mPositiveTorqueVerticalAngleCWMin);
                        ////}

                        ////LogMessage("TODOTEST: clipped verticalAlpha=", mDirectionSearchCurrent.angleCw(Vertical));
                    }



                    // TODOTEST: Gradient descent
                    ////{
                    ////    //
                    ////    // Gradient descent, minimizing torque
                    ////    //

                    ////    float constexpr DAlpha = 0.005f;

                    ////    ////// Positive
                    ////    ////vec2f const positiveStepDirection = mDirectionSearchCurrent.rotate(DAlpha);
                    ////    ////auto [positiveStepTotalBuoyantForce, positiveStepCoB] = CalculateBuoyancy(mWaterline->Center, positiveStepDirection);
                    ////    ////float const positiveStepTorque = positiveStepDirection.cross((positiveStepCoB - mStaticResults->CenterOfMass));

                    ////    ////// Negative
                    ////    ////vec2f const negativeStepDirection = mDirectionSearchCurrent.rotate(-DAlpha);
                    ////    ////auto [negativeStepTotalBuoyantForce, negativeStepCoB] = CalculateBuoyancy(mWaterline->Center, negativeStepDirection);
                    ////    ////float const negativeStepTorque = negativeStepDirection.cross((negativeStepCoB - mStaticResults->CenterOfMass));

                    ////    ////// Choose step that minimizes torque the most
                    ////    ////if ((torque - positiveStepTorque) > (torque - negativeStepTorque)

                    ////    vec2f const stepDirection = mDirectionSearchCurrent.rotate(DAlpha);
                    ////    auto [_, stepCoB] = CalculateBuoyancy(mWaterline->Center, stepDirection);
                    ////    float const stepTorque = stepDirection.cross((stepCoB - mStaticResults->CenterOfMass));

                    ////    // Get closer to zero
                    ////    if (std::abs(stepTorque) <= std::abs(torque))
                    ////    {
                    ////        // Direction of step minimizes (absolute value of) torque
                    ////        mDirectionSearchCurrent = mDirectionSearchCurrent.rotate(DAlpha);
                    ////    }
                    ////    else
                    ////    {
                    ////        // Direction of step gives larger (absolute) value
                    ////        mDirectionSearchCurrent = mDirectionSearchCurrent.rotate(-DAlpha);
                    ////    }
                    ////}

                    //
                    // Restart search from here
                    //

                    std::tie(mLevelSearchLowest, mLevelSearchHighest) = CalculateLevelSearchLimits(
                        mStaticResults->CenterOfMass,
                        mDirectionSearchCurrent);

                    // TODOTEST: restart from here
                    //mLevelSearchCurrent = 0.0f;
                    assert(mLevelSearchCurrent <= mLevelSearchLowest && mLevelSearchCurrent >= mLevelSearchHighest);

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

std::tuple<float, vec2f> WaterlineAnalyzer::CalculateBuoyancy(
    vec2f const & waterlineCenter,
    vec2f const & waterlineDirection)
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
                float const alignment = (coordsF - waterlineCenter).dot(waterlineDirection);
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