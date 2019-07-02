/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>
#include <GameCore/Log.h>
#include <GameCore/PrecalculatedFunction.h>

#include <cmath>
#include <limits>

namespace Physics {

void Points::Add(
    vec2f const & position,
    StructuralMaterial const & structuralMaterial,
    ElectricalMaterial const * electricalMaterial,
    bool isRope,
    ElementIndex electricalElementIndex,
    bool isLeaking,
    vec4f const & color,
    vec2f const & textureCoordinates)
{
    ElementIndex const pointIndex = static_cast<ElementIndex>(mMaterialsBuffer.GetCurrentPopulatedSize());

    mMaterialsBuffer.emplace_back(&structuralMaterial, electricalMaterial);
    mIsRopeBuffer.emplace_back(isRope);

    mPositionBuffer.emplace_back(position);
    mVelocityBuffer.emplace_back(vec2f::zero());
    mForceBuffer.emplace_back(vec2f::zero());
    mAugmentedMaterialMassBuffer.emplace_back(structuralMaterial.GetMass());
    mMassBuffer.emplace_back(structuralMaterial.GetMass());
    mDecayBuffer.emplace_back(1.0f);
    mIntegrationFactorTimeCoefficientBuffer.emplace_back(CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations));

    mIntegrationFactorBuffer.emplace_back(vec2f::zero());
    mForceRenderBuffer.emplace_back(vec2f::zero());

    mMaterialIsHullBuffer.emplace_back(structuralMaterial.IsHull);
    mMaterialWaterVolumeFillBuffer.emplace_back(structuralMaterial.WaterVolumeFill);
    mMaterialWaterIntakeBuffer.emplace_back(structuralMaterial.WaterIntake);
    mMaterialWaterRestitutionBuffer.emplace_back(1.0f - structuralMaterial.WaterRetention);
    mMaterialWaterDiffusionSpeedBuffer.emplace_back(structuralMaterial.WaterDiffusionSpeed);

    mWaterBuffer.emplace_back(0.0f);
    mWaterVelocityBuffer.emplace_back(vec2f::zero());
    mWaterMomentumBuffer.emplace_back(vec2f::zero());
    mCumulatedIntakenWater.emplace_back(0.0f);
    mIsLeakingBuffer.emplace_back(isLeaking);
    if (isLeaking)
        SetLeaking(pointIndex);
    mFactoryIsLeakingBuffer.emplace_back(isLeaking);

    // Heat dynamics
    mTemperatureBuffer.emplace_back(GameParameters::AirTemperature);
    mMaterialHeatCapacityBuffer.emplace_back(structuralMaterial.GetHeatCapacity());
    mMaterialIgnitionTemperatureBuffer.emplace_back(structuralMaterial.IgnitionTemperature);
    mCombustionStateBuffer.emplace_back(CombustionState());

    // Electrical dynamics
    mElectricalElementBuffer.emplace_back(electricalElementIndex);
    mLightBuffer.emplace_back(0.0f);

    // Wind dynamics
    mMaterialWindReceptivityBuffer.emplace_back(structuralMaterial.WindReceptivity);

    // Rust dynamics
    mMaterialRustReceptivityBuffer.emplace_back(structuralMaterial.RustReceptivity);

    // Ephemeral particles
    mEphemeralTypeBuffer.emplace_back(EphemeralType::None);
    mEphemeralStartTimeBuffer.emplace_back(0.0f);
    mEphemeralMaxLifetimeBuffer.emplace_back(0.0f);
    mEphemeralStateBuffer.emplace_back(EphemeralState::DebrisState());

    // Structure
    mConnectedSpringsBuffer.emplace_back();
    mFactoryConnectedSpringsBuffer.emplace_back();
    mConnectedTrianglesBuffer.emplace_back();
    mFactoryConnectedTrianglesBuffer.emplace_back();

    mConnectedComponentIdBuffer.emplace_back(NoneConnectedComponentId);
    mPlaneIdBuffer.emplace_back(NonePlaneId);
    mPlaneIdFloatBuffer.emplace_back(0.0f);
    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back();

    mIsPinnedBuffer.emplace_back(false);

    mRepairStateBuffer.emplace_back();

    mColorBuffer.emplace_back(color);
    mTextureCoordinatesBuffer.emplace_back(textureCoordinates);
}

void Points::CreateEphemeralParticleAirBubble(
    vec2f const & position,
    float initialSize,
    float vortexAmplitude,
    float vortexPeriod,
    StructuralMaterial const & structuralMaterial,
    float currentSimulationTime,
    PlaneId planeId)
{
    // Get a free slot (but don't steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, false);
    if (NoneElementIndex == pointIndex)
        return; // No luck

    //
    // Store attributes
    //

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = vec2f::zero();
    mForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mDecayBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mMaterialWaterVolumeFillBuffer[pointIndex] = structuralMaterial.WaterVolumeFill;
    mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mTemperatureBuffer[pointIndex] = GameParameters::AirTemperature;
    mMaterialHeatCapacityBuffer[pointIndex] = structuralMaterial.GetHeatCapacity();
    mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    mCombustionStateBuffer[pointIndex] = CombustionState();

    mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 0.0f;

    mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::AirBubble;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::numeric_limits<float>::max();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::AirBubbleState(
        GameRandomEngine::GetInstance().Choose<TextureFrameIndex>(2),
        initialSize,
        vortexAmplitude,
        vortexPeriod);

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor;
}

void Points::CreateEphemeralParticleDebris(
    vec2f const & position,
    vec2f const & velocity,
    StructuralMaterial const & structuralMaterial,
    float currentSimulationTime,
    std::chrono::milliseconds maxLifetime,
    PlaneId planeId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, true);
    assert(NoneElementIndex != pointIndex);

    //
    // Store attributes
    //

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mDecayBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mMaterialWaterVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mTemperatureBuffer[pointIndex] = GameParameters::AirTemperature;
    mMaterialHeatCapacityBuffer[pointIndex] = structuralMaterial.GetHeatCapacity();
    mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    mCombustionStateBuffer[pointIndex] = CombustionState();

    mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 3.0f; // Debris are susceptible to wind

    mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::Debris;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::DebrisState();

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor;

    // Remember that ephemeral points are dirty now
    mAreEphemeralPointsDirty = true;
}

void Points::CreateEphemeralParticleSparkle(
    vec2f const & position,
    vec2f const & velocity,
    StructuralMaterial const & structuralMaterial,
    float currentSimulationTime,
    std::chrono::milliseconds maxLifetime,
    PlaneId planeId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, true);
    assert(NoneElementIndex != pointIndex);

    //
    // Store attributes
    //

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mDecayBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mMaterialWaterVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mTemperatureBuffer[pointIndex] = 773.15f; // 500 Celsius, arbitrary
    mMaterialHeatCapacityBuffer[pointIndex] = structuralMaterial.GetHeatCapacity();
    mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    mCombustionStateBuffer[pointIndex] = CombustionState();

    mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 5.0f; // Sparkles are susceptible to wind

    mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::Sparkle;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::SparkleState(
        GameRandomEngine::GetInstance().Choose<TextureFrameIndex>(2));

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);
}

void Points::Detach(
    ElementIndex pointElementIndex,
    vec2f const & velocity,
    DetachOptions detachOptions,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Invoke detach handler
    if (!!mDetachHandler)
    {
        mDetachHandler(
            pointElementIndex,
            !!(detachOptions & Points::DetachOptions::GenerateDebris),
            currentSimulationTime,
            gameParameters);
    }

    // Imprint velocity, unless the point is pinned
    if (!mIsPinnedBuffer[pointElementIndex])
    {
        mVelocityBuffer[pointElementIndex] = velocity;
    }
}

void Points::DestroyEphemeralParticle(
    ElementIndex pointElementIndex)
{
    // Invoke handler
    if (!!mEphemeralParticleDestroyHandler)
    {
        mEphemeralParticleDestroyHandler(pointElementIndex);
    }

    // Fire destroy event
    mGameEventHandler->OnDestroy(
        GetStructuralMaterial(pointElementIndex),
        mParentWorld.IsUnderwater(GetPosition(pointElementIndex)),
        1u);

    // Expire particle
    ExpireEphemeralParticle(pointElementIndex);
}

void Points::UpdateGameParameters(GameParameters const & gameParameters)
{
    //
    // Check parameter changes
    //

    float const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<float>();
    if (numMechanicalDynamicsIterations != mCurrentNumMechanicalDynamicsIterations)
    {
        // Recalc integration factor time coefficients
        for (ElementIndex i : *this)
        {
            mIntegrationFactorTimeCoefficientBuffer[i] = CalculateIntegrationFactorTimeCoefficient(numMechanicalDynamicsIterations);
        }

        // Remember the new values
        mCurrentNumMechanicalDynamicsIterations = numMechanicalDynamicsIterations;
    }

    float const cumulatedIntakenWaterThresholdForAirBubbles = gameParameters.CumulatedIntakenWaterThresholdForAirBubbles;
    if (cumulatedIntakenWaterThresholdForAirBubbles != mCurrentCumulatedIntakenWaterThresholdForAirBubbles)
    {
        // Randomize cumulated water intaken for each leaking point
        for (ElementIndex i : NonEphemeralPoints())
        {
            if (IsLeaking(i))
            {
                mCumulatedIntakenWater[i] = RandomizeCumulatedIntakenWater(cumulatedIntakenWaterThresholdForAirBubbles);
            }
        }

        // Remember the new values
        mCurrentCumulatedIntakenWaterThresholdForAirBubbles = cumulatedIntakenWaterThresholdForAirBubbles;
    }
}

void Points::UpdateCombustionLowFrequency(
    float /*currentSimulationTime*/,
    float dt,
    GameParameters const & gameParameters)
{
    //
    // Take care of following:
    // - NotBurning->Burning transition
    // - Burning->Extinguishing transition
    // - Extinguishing->NotBurning transition

    // Prepare candidates for ignition; we'll pick the top N ones
    // based on the ignition temperature delta
    mIgnitionCandidates.clear();

    // TODO: move to game parameters

    static constexpr float IgnitionTemperatureHighWatermark = 20.0f;
    static constexpr float IgnitionTemperatureLowWatermark = -20.0f;

    static constexpr float SmotheringWaterLowWatermark = 0.05f;
    static constexpr float SmotheringWaterHighWatermark = 0.1f;

    static constexpr float SmotheringDecayLowWatermark = 0.01f;
    static constexpr float SmotheringDecayHighWatermark = 0.1f;

    // Decay rate
    float const effectiveCombustionDecayRate = (30.0f / gameParameters.CombustionSpeedAdjustment / dt);

    // No real reason not to do ephemeral points as well, other than they're
    // currently not expected to burn
    for (auto const pointIndex : this->NonEphemeralPoints())
    {
        switch (mCombustionStateBuffer[pointIndex].State)
        {
            case CombustionState::StateType::NotBurning:
            {
                float const effectiveIgnitionTemperature = mMaterialIgnitionTemperatureBuffer[pointIndex] * gameParameters.IgnitionTemperatureAdjustment;

                // See if this point should start burning
                if (GetTemperature(pointIndex) >= effectiveIgnitionTemperature + IgnitionTemperatureHighWatermark
                    && !mParentWorld.IsUnderwater(GetPosition(pointIndex))
                    && GetWater(pointIndex) < SmotheringWaterLowWatermark
                    && GetDecay(pointIndex) > SmotheringDecayHighWatermark
                    && mBurningPoints.size() < GameParameters::MaxBurningParticles)
                {
                    // Store point as candidate
                    mIgnitionCandidates.emplace_back(
                        pointIndex,
                        (GetTemperature(pointIndex) - effectiveIgnitionTemperature) / effectiveIgnitionTemperature);
                }

                break;
            }

            case CombustionState::StateType::Burning:
            {
                // See if this point should start extinguishing...

                // ...for water or sea: we do this check at high frequency

                // ...for temperature or decay
                if (GetTemperature(pointIndex) <= mMaterialIgnitionTemperatureBuffer[pointIndex] * gameParameters.IgnitionTemperatureAdjustment + IgnitionTemperatureLowWatermark
                    || GetDecay(pointIndex) < SmotheringDecayLowWatermark)
                {
                    //
                    // Transition to Extinguishing - by consumption
                    //

                    mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Extinguishing_Consumed;

                    // Notify combustion end
                    mGameEventHandler->OnPointCombustionEnd();
                }
                else
                {
                    // Apply effects of burning

                    //
                    // 1. Decay - proportionally to mass
                    //
                    // Our goal:
                    // - An iron hull mass (750Kg) decays completely (goes to 0.01)
                    //   in 30 (simulated) seconds
                    // - A smaller (larger) mass decays in shorter (longer) time,
                    //   but a very small mass shouldn't burn in too short of a time
                    //

                    float const massMultiplier = pow(
                        mMaterialsBuffer[pointIndex].Structural->GetMass() / 750.0f,
                        0.3f); // Magic number: one tenth of the mass is half the time

                    float const totalDecaySteps =
                        effectiveCombustionDecayRate
                        * massMultiplier;

                    float const alpha = pow(0.01f, 1.0f / totalDecaySteps);

                    // Decay point
                    mDecayBuffer[pointIndex] *= alpha;
                }

                break;
            }

            case CombustionState::StateType::Extinguishing_Consumed:
            case CombustionState::StateType::Extinguishing_Smothered:
            {
                // See if this point should stop extinguishing
                if (mCombustionStateBuffer[pointIndex].FlameDevelopment == 0.0f)
                {
                    //
                    // Stop burning
                    //

                    mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::NotBurning;

                    // Remove point from set of burning points
                    auto pointIt = std::find(
                        mBurningPoints.cbegin(),
                        mBurningPoints.cend(),
                        pointIndex);
                    assert(pointIt != mBurningPoints.cend());
                    mBurningPoints.erase(pointIt);
                }

                break;
            }
        }
    }

    //
    // Now pick candidates for ignition
    //

    // Sort candidates by ignition temperature delta
    mIgnitionCandidates.sort(
        [](auto const & t1, auto const & t2)
        {
            return std::get<1>(t1) > std::get<1>(t2);
        });

    // Ignite the first N points
    // TODO: find good value, depends likely also on step size
    size_t constexpr MaxPointsIgnitedPerIteration = 10;
    for (size_t i = 0; i < mIgnitionCandidates.size() && i < MaxPointsIgnitedPerIteration; ++i)
    {
        auto const pointIndex = std::get<0>(mIgnitionCandidates[i]);

        //
        // Start burning
        //

        mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Burning;
        mCombustionStateBuffer[pointIndex].FlameDevelopment = 0.0f;

        // Add point to vector of burning points, sorted by plane ID
        assert(mBurningPoints.cend() == std::find(mBurningPoints.cbegin(), mBurningPoints.cend(), pointIndex));
        mBurningPoints.insert(
            std::upper_bound(
                mBurningPoints.cbegin(),
                mBurningPoints.cend(),
                pointIndex,
                [this](auto p1, auto p2)
                {
                    return this->mPlaneIdBuffer[p1] < mPlaneIdBuffer[p2];
                }),
            pointIndex);

        // Notify
        mGameEventHandler->OnPointCombustionBegin();
    }
}

void Points::UpdateCombustionHighFrequency(
    float /*currentSimulationTime*/,
    float dt,
    GameParameters const & gameParameters)
{
    //
    // For all burning points, take care of following:
    // - Burning points: development up
    // - Extinguishing points: development down
    // - Burning points: heat generation
    //

    float constexpr DevelopmentTime = 1.0f; // Seconds to full development
    float constexpr ExtinguishingConsumedTime = 1.5f; // Second to full extinguishment, when consumed
    float constexpr ExtinguishingSmotheredTime = 0.4f; // Second to full extinguishment, when smothered

    // Heat generated by combustion
    float const effectiveCombustionHeat =
        100.0f * 1000.0f
        * GameParameters::SimulationStepTimeDuration<float>
        * gameParameters.CombustionHeatAdjustment;

    for (auto const pointIndex : mBurningPoints)
    {
        switch (mCombustionStateBuffer[pointIndex].State)
        {
            case CombustionState::StateType::Burning:
            case CombustionState::StateType::Extinguishing_Consumed:
            {
                //
                // Check if this point should stop burning or start extinguishing faster
                //

                // TODO: move to game parameters
                static constexpr float SmotheringWaterLowWatermark = 0.05f;
                static constexpr float SmotheringWaterHighWatermark = 0.1f;

                if (mParentWorld.IsUnderwater(GetPosition(pointIndex))
                    || GetWater(pointIndex) > SmotheringWaterHighWatermark)
                {
                    //
                    // Transition to Extinguishing - by smothering
                    //

                    // Notify combustion end - if we are burning
                    if (mCombustionStateBuffer[pointIndex].State == CombustionState::StateType::Burning)
                        mGameEventHandler->OnPointCombustionEnd();

                    // Transition
                    mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Extinguishing_Smothered;

                    // Notify sizzling
                    mGameEventHandler->OnCombustionSmothered();
                }
                else if (mCombustionStateBuffer[pointIndex].State == CombustionState::StateType::Burning)
                {
                    //
                    // Develop flames
                    //

                    mCombustionStateBuffer[pointIndex].FlameDevelopment = std::min(
                        1.0f,
                        mCombustionStateBuffer[pointIndex].FlameDevelopment + dt / DevelopmentTime);

                    //
                    // Generate heat at:
                    // - point itself: fix to constant temperature = ignition temperature + 10%
                    // - neighbors: 100Kw * C, scaled by directional alpha
                    //

                    mTemperatureBuffer[pointIndex] =
                        mMaterialIgnitionTemperatureBuffer[pointIndex]
                        * gameParameters.IgnitionTemperatureAdjustment
                        * 1.1f;

                    for (auto const s : GetConnectedSprings(pointIndex).ConnectedSprings)
                    {
                        auto const otherEndpointIndex = s.OtherEndpointIndex;

                        // Calculate direction coefficient so to prefer upwards direction:
                        // 0.2 + 1.5*(1 - cos(theta)): 3.2 N, 0.2 S, 1.7 W and E
                        vec2f const springDir = (GetPosition(otherEndpointIndex) - GetPosition(pointIndex)).normalise();
                        float const dirAlpha =
                            (0.2f + 1.5f * (1.0f - springDir.dot(GameParameters::GravityNormalized)));
                            // No normalization: if using normalization flame does not propagate along rope

                        // Add heat to point
                        mTemperatureBuffer[otherEndpointIndex] +=
                            effectiveCombustionHeat
                            * dirAlpha
                            / mMaterialHeatCapacityBuffer[otherEndpointIndex];
                    }
                }
                else if (mCombustionStateBuffer[pointIndex].State == CombustionState::StateType::Extinguishing_Consumed)
                {
                    //
                    // Un-develop flames
                    //

                    mCombustionStateBuffer[pointIndex].FlameDevelopment = std::max(
                        0.0f,
                        mCombustionStateBuffer[pointIndex].FlameDevelopment - dt / ExtinguishingConsumedTime);
                }

                break;
            }

            case CombustionState::StateType::NotBurning:
            {
                // Nothing
                break;
            }

            case CombustionState::StateType::Extinguishing_Smothered:
            {
                //
                // Un-develop flames
                //

                mCombustionStateBuffer[pointIndex].FlameDevelopment = std::max(
                    0.0f,
                    mCombustionStateBuffer[pointIndex].FlameDevelopment - dt / ExtinguishingSmotheredTime);

                break;
            }
        }
    }
}

void Points::ReorderBurningPointsForDepth()
{
    std::sort(
        mBurningPoints.begin(),
        mBurningPoints.end(),
        [this](auto p1, auto p2)
        {
            return this->mPlaneIdBuffer[p1] < this->mPlaneIdBuffer[p2];
        });
}

void Points::UpdateEphemeralParticles(
    float currentSimulationTime,
    GameParameters const & /*gameParameters*/)
{
    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        auto const ephemeralType = GetEphemeralType(pointIndex);
        if (EphemeralType::None != ephemeralType)
        {
            //
            // Run this particle's state machine
            //

            switch (ephemeralType)
            {
                case EphemeralType::AirBubble:
                {
                    // Do not advance air bubble if it's pinned
                    if (!mIsPinnedBuffer[pointIndex])
                    {
                        float const waterHeight = mParentWorld.GetOceanSurfaceHeightAt(GetPosition(pointIndex).x);
                        float const deltaY = waterHeight - GetPosition(pointIndex).y;

                        if (deltaY <= 0.0f)
                        {
                            // Got to the surface, expire
                            ExpireEphemeralParticle(pointIndex);
                        }
                        else
                        {
                            //
                            // Update progress based off y
                            //

                            mEphemeralStateBuffer[pointIndex].AirBubble.CurrentDeltaY = deltaY;

                            mEphemeralStateBuffer[pointIndex].AirBubble.Progress =
                                -1.0f
                                / (-1.0f + std::min(GetPosition(pointIndex).y, 0.0f));

                            //
                            // Update vortex
                            //

                            float const lifetime = currentSimulationTime - mEphemeralStartTimeBuffer[pointIndex];

                            float const vortexAmplitude =
                                mEphemeralStateBuffer[pointIndex].AirBubble.VortexAmplitude
                                + mEphemeralStateBuffer[pointIndex].AirBubble.Progress;

                            float vortexValue =
                                vortexAmplitude
                                * PrecalcLoFreqSin.GetNearestPeriodic(mEphemeralStateBuffer[pointIndex].AirBubble.NormalizedVortexAngularVelocity * lifetime);

                            // Update position
                            mPositionBuffer[pointIndex].x +=
                                vortexValue - mEphemeralStateBuffer[pointIndex].AirBubble.LastVortexValue;

                            mEphemeralStateBuffer[pointIndex].AirBubble.LastVortexValue = vortexValue;
                        }
                    }

                    break;
                }

                case EphemeralType::Debris:
                {
                    // Check if expired
                    auto const elapsedLifetime = currentSimulationTime - mEphemeralStartTimeBuffer[pointIndex];
                    if (elapsedLifetime >= mEphemeralMaxLifetimeBuffer[pointIndex])
                    {
                        ExpireEphemeralParticle(pointIndex);

                        // Remember that ephemeral points are now dirty
                        mAreEphemeralPointsDirty = true;
                    }
                    else
                    {
                        // Update alpha based off remaining time

                        float alpha = std::max(
                            1.0f - elapsedLifetime / mEphemeralMaxLifetimeBuffer[pointIndex],
                            0.0f);

                        mColorBuffer[pointIndex].w = alpha;
                    }

                    break;
                }

                case EphemeralType::Sparkle:
                {
                    // Check if expired
                    auto const elapsedLifetime = currentSimulationTime - mEphemeralStartTimeBuffer[pointIndex];
                    if (elapsedLifetime >= mEphemeralMaxLifetimeBuffer[pointIndex])
                    {
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        // Update progress based off remaining time

                        mEphemeralStateBuffer[pointIndex].Sparkle.Progress =
                            elapsedLifetime / mEphemeralMaxLifetimeBuffer[pointIndex];
                    }

                    break;
                }

                default:
                {
                    // Do nothing
                }
            }
        }
    }
}

void Points::Query(ElementIndex pointElementIndex) const
{
    LogMessage("PointIndex: ", pointElementIndex);
    LogMessage("P=", mPositionBuffer[pointElementIndex].toString(), " V=", mVelocityBuffer[pointElementIndex].toString());
    LogMessage("W=", mWaterBuffer[pointElementIndex], " T=", mTemperatureBuffer[pointElementIndex], " Decay=", mDecayBuffer[pointElementIndex]);
    LogMessage("Springs: ", mConnectedSpringsBuffer[pointElementIndex].ConnectedSprings.size(), " (factory: ", mFactoryConnectedSpringsBuffer[pointElementIndex].ConnectedSprings.size(), ")");
    LogMessage("PlaneID: ", mPlaneIdBuffer[pointElementIndex]);
    LogMessage("ConnectedComponentID: ", mConnectedComponentIdBuffer[pointElementIndex]);
}

void Points::UploadAttributes(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    // Upload immutable attributes, if we haven't uploaded them yet
    if (mIsTextureCoordinatesBufferDirty)
    {
        renderContext.UploadShipPointImmutableAttributes(
            shipId,
            mTextureCoordinatesBuffer.data());

        mIsTextureCoordinatesBufferDirty = false;
    }

    // Upload colors, if dirty
    if (mIsWholeColorBufferDirty)
    {
        renderContext.UploadShipPointColors(
            shipId,
            mColorBuffer.data(),
            0,
            mAllPointCount);

        mIsWholeColorBufferDirty = false;
    }
    else
    {
        // Only upload ephemeral particle portion
        renderContext.UploadShipPointColors(
            shipId,
            &(mColorBuffer.data()[mShipPointCount]),
            mShipPointCount,
            mEphemeralPointCount);
    }

    //
    // Upload mutable attributes
    //

    renderContext.UploadShipPointMutableAttributesStart(shipId);

    renderContext.UploadShipPointMutableAttributes(
        shipId,
        mPositionBuffer.data(),
        mLightBuffer.data(),
        mWaterBuffer.data());

    if (mIsPlaneIdBufferNonEphemeralDirty)
    {
        if (mIsPlaneIdBufferEphemeralDirty)
        {
            // Whole

            renderContext.UploadShipPointMutableAttributesPlaneId(
                shipId,
                mPlaneIdFloatBuffer.data(),
                0,
                mAllPointCount);

            mIsPlaneIdBufferEphemeralDirty = false;
        }
        else
        {
            // Just non-ephemeral portion

            renderContext.UploadShipPointMutableAttributesPlaneId(
                shipId,
                mPlaneIdFloatBuffer.data(),
                0,
                mShipPointCount);
        }

        mIsPlaneIdBufferNonEphemeralDirty = false;
    }
    else if (mIsPlaneIdBufferEphemeralDirty)
    {
        // Just ephemeral portion

        renderContext.UploadShipPointMutableAttributesPlaneId(
            shipId,
            &(mPlaneIdFloatBuffer.data()[mShipPointCount]),
            mShipPointCount,
            mEphemeralPointCount);

        mIsPlaneIdBufferEphemeralDirty = false;
    }

    if (mIsDecayBufferDirty)
    {
        renderContext.UploadShipPointMutableAttributesDecay(
            shipId,
            mDecayBuffer.data(),
            0,
            mAllPointCount);

        mIsDecayBufferDirty = false;
    }

    if (mIsTemperatureBufferDirty
        && renderContext.GetDrawHeatOverlay())
    {
        renderContext.UploadShipPointTemperature(
            shipId,
            mTemperatureBuffer.data(),
            0,
            mAllPointCount);

        mIsTemperatureBufferDirty = false;
    }

    renderContext.UploadShipPointMutableAttributesEnd(shipId);
}

void Points::UploadNonEphemeralPointElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    bool doUploadAllPoints = (DebugShipRenderMode::Points == renderContext.GetDebugShipRenderMode());

    for (ElementIndex pointIndex : NonEphemeralPoints())
    {
        if (doUploadAllPoints
            || mConnectedSpringsBuffer[pointIndex].ConnectedSprings.empty()) // orphaned
        {
            renderContext.UploadShipElementPoint(
                shipId,
                pointIndex);
        }
    }
}

void Points::UploadFlames(
    ShipId shipId,
    float windSpeedMagnitude,
    Render::RenderContext & renderContext) const
{
    if (renderContext.GetShipFlameRenderMode() != ShipFlameRenderMode::NoDraw)
    {
        renderContext.UploadShipFlamesStart(shipId, windSpeedMagnitude);

        // Upload flames, in order of plane ID
        for (auto const pointIndex : mBurningPoints)
        {
            renderContext.UploadShipFlame(
                shipId,
                GetPlaneId(pointIndex),
                GetPosition(pointIndex),
                mCombustionStateBuffer[pointIndex].FlameDevelopment,
                static_cast<float>(pointIndex)); // Personality
        }

        renderContext.UploadShipFlamesEnd(shipId);
    }
}

void Points::UploadVectors(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    static constexpr vec4f VectorColor(0.5f, 0.1f, 0.f, 1.0f);

    if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointVelocity)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdFloatBuffer.data(),
            mVelocityBuffer.data(),
            0.25f,
            VectorColor);
    }
    else if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointForce)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdFloatBuffer.data(),
            mForceRenderBuffer.data(),
            0.0005f,
            VectorColor);
    }
    else if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointWaterVelocity)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdFloatBuffer.data(),
            mWaterVelocityBuffer.data(),
            1.0f,
            VectorColor);
    }
    else if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointWaterMomentum)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdFloatBuffer.data(),
            mWaterMomentumBuffer.data(),
            0.4f,
            VectorColor);
    }
}

void Points::UploadEphemeralParticles(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    //
    // Upload points and/or textures
    //

    if (mAreEphemeralPointsDirty)
    {
        renderContext.UploadShipElementEphemeralPointsStart(shipId);
    }

    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        switch (GetEphemeralType(pointIndex))
        {
            case EphemeralType::AirBubble:
            {
                renderContext.UploadShipAirBubble(
                    shipId,
                    GetPlaneId(pointIndex),
                    TextureFrameId(TextureGroupType::AirBubble, mEphemeralStateBuffer[pointIndex].AirBubble.FrameIndex),
                    GetPosition(pointIndex),
                    mEphemeralStateBuffer[pointIndex].AirBubble.InitialSize, // Scale
                    std::min(1.0f, mEphemeralStateBuffer[pointIndex].AirBubble.CurrentDeltaY / 4.0f)); // Alpha

                break;
            }

            case EphemeralType::Debris:
            {
                // Don't upload point unless there's been a change
                if (mAreEphemeralPointsDirty)
                {
                    renderContext.UploadShipElementEphemeralPoint(
                        shipId,
                        pointIndex);
                }

                break;
            }

            case EphemeralType::Sparkle:
            {
                renderContext.UploadShipGenericTextureRenderSpecification(
                    shipId,
                    GetPlaneId(pointIndex),
                    TextureFrameId(TextureGroupType::SawSparkle, mEphemeralStateBuffer[pointIndex].Sparkle.FrameIndex),
                    GetPosition(pointIndex),
                    1.0f,
                    4.0f * mEphemeralStateBuffer[pointIndex].Sparkle.Progress,
                    1.0f - mEphemeralStateBuffer[pointIndex].Sparkle.Progress);

                break;
            }

            case EphemeralType::None:
            default:
            {
                // Ignore
                break;
            }
        }
    }

    if (mAreEphemeralPointsDirty)
    {
        renderContext.UploadShipElementEphemeralPointsEnd(shipId);

        mAreEphemeralPointsDirty = false;
    }
}

void Points::AugmentMaterialMass(
    ElementIndex pointElementIndex,
    float offset,
    Springs & springs)
{
    assert(pointElementIndex < mElementCount);

    mAugmentedMaterialMassBuffer[pointElementIndex] =
        GetStructuralMaterial(pointElementIndex).GetMass()
        + offset;

    // Notify all connected springs
    for (auto connectedSpring : mConnectedSpringsBuffer[pointElementIndex].ConnectedSprings)
    {
        springs.OnEndpointMassUpdated(connectedSpring.SpringIndex, *this);
    }
}

void Points::UpdateMasses(GameParameters const & gameParameters)
{
    //
    // Update:
    //  - CurrentMass: augmented material mass + point's water mass
    //  - Integration factor: integration factor time coefficient / total mass
    //

    float const densityAdjustedWaterMass = GameParameters::WaterMass * gameParameters.WaterDensityAdjustment;

    for (ElementIndex i : *this)
    {
        float const mass =
            mAugmentedMaterialMassBuffer[i]
            + std::min(GetWater(i), GetMaterialWaterVolumeFill(i)) * densityAdjustedWaterMass;

        assert(mass > 0.0f);

        mMassBuffer[i] = mass;

        mIntegrationFactorBuffer[i] = vec2f(
            mIntegrationFactorTimeCoefficientBuffer[i] / mass,
            mIntegrationFactorTimeCoefficientBuffer[i] / mass);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ElementIndex Points::FindFreeEphemeralParticle(
    float currentSimulationTime,
    bool force)
{
    //
    // Search for the firt free ephemeral particle; if a free one is not found, reuse the
    // oldest particle
    //

    ElementIndex oldestParticle = NoneElementIndex;
    float oldestParticleLifetime = 0.0f;

    assert(mFreeEphemeralParticleSearchStartIndex >= mShipPointCount
        && mFreeEphemeralParticleSearchStartIndex < mAllPointCount);

    for (ElementIndex p = mFreeEphemeralParticleSearchStartIndex; ; /*incremented in loop*/)
    {
        if (EphemeralType::None == GetEphemeralType(p))
        {
            // Found!

            // Remember to start after this one next time
            mFreeEphemeralParticleSearchStartIndex = p + 1;
            if (mFreeEphemeralParticleSearchStartIndex >= mAllPointCount)
                mFreeEphemeralParticleSearchStartIndex = mShipPointCount;

            return p;
        }

        // Check whether it's the oldest
        auto lifetime = currentSimulationTime - mEphemeralStartTimeBuffer[p];
        if (lifetime >= oldestParticleLifetime)
        {
            oldestParticle = p;
            oldestParticleLifetime = lifetime;
        }

        // Advance
        ++p;
        if (p >= mAllPointCount)
            p = mShipPointCount;

        if (p == mFreeEphemeralParticleSearchStartIndex)
        {
            // Went around
            break;
        }
    }

    //
    // No luck
    //

    if (!force)
        return NoneElementIndex;


    //
    // Steal the oldest
    //

    assert(NoneElementIndex != oldestParticle);

    // Remember to start after this one next time
    mFreeEphemeralParticleSearchStartIndex = oldestParticle + 1;
    if (mFreeEphemeralParticleSearchStartIndex >= mAllPointCount)
        mFreeEphemeralParticleSearchStartIndex = mShipPointCount;

    return oldestParticle;
}

}