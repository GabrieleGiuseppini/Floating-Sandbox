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
    vec2f const & textureCoordinates,
	float randomNormalizedUniformFloat)
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
    mFrozenCoefficientBuffer.emplace_back(1.0f);
    mIntegrationFactorTimeCoefficientBuffer.emplace_back(CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f));

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
    mTemperatureBuffer.emplace_back(GameParameters::InitialTemperature);
    mMaterialHeatCapacityBuffer.emplace_back(structuralMaterial.GetHeatCapacity());
    mMaterialIgnitionTemperatureBuffer.emplace_back(structuralMaterial.IgnitionTemperature);
    mMaterialCombustionTypeBuffer.emplace_back(structuralMaterial.CombustionType);
    mCombustionStateBuffer.emplace_back(CombustionState());

    // Electrical dynamics
    mElectricalElementBuffer.emplace_back(electricalElementIndex);
    mLightBuffer.emplace_back(0.0f);

    // Wind dynamics
    mMaterialWindReceptivityBuffer.emplace_back(structuralMaterial.WindReceptivity);

    // Rust dynamics
    mMaterialRustReceptivityBuffer.emplace_back(structuralMaterial.RustReceptivity);

    // Ephemeral particles
    mEphemeralParticleAttributes1Buffer.emplace_back();
    mEphemeralParticleAttributes2Buffer.emplace_back();

    // Structure
    mConnectedSpringsBuffer.emplace_back();
    mFactoryConnectedSpringsBuffer.emplace_back();
    mConnectedTrianglesBuffer.emplace_back();
    mFactoryConnectedTrianglesBuffer.emplace_back();

	// Connectivity
    mConnectedComponentIdBuffer.emplace_back(NoneConnectedComponentId);
    mPlaneIdBuffer.emplace_back(NonePlaneId);
    mPlaneIdFloatBuffer.emplace_back(0.0f);
    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back();

	// Repair state
    mRepairStateBuffer.emplace_back();

	// Randomness
	mRandomNormalizedUniformFloatBuffer.emplace_back(randomNormalizedUniformFloat);

	// Immutable render attributes
    mColorBuffer.emplace_back(color);
    mTextureCoordinatesBuffer.emplace_back(textureCoordinates);
}

void Points::CreateEphemeralParticleAirBubble(
    vec2f const & position,
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
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mMaterialWaterVolumeFillBuffer[pointIndex] = structuralMaterial.WaterVolumeFill;
    //mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    assert(mWaterBuffer[pointIndex] == 0.0f);
    //mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);
    //mIsLeakingBuffer[pointIndex] = false;

    assert(mTemperatureBuffer[pointIndex] == 0.0f);
    //mTemperatureBuffer[pointIndex] = GameParameters::InitialTemperature;
    //mMaterialHeatCapacityBuffer[pointIndex] = structuralMaterial.GetHeatCapacity();
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = structuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 0.0f; // Air bubbles (underwater) do not care about wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::AirBubble;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxLifetime = std::numeric_limits<float>::max();
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::AirBubbleState(
        vortexAmplitude,
        vortexPeriod);

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

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
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mMaterialWaterVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    //mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    assert(mWaterBuffer[pointIndex] == 0.0f);
    //mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);
    //mIsLeakingBuffer[pointIndex] = false;

    assert(mTemperatureBuffer[pointIndex] == 0.0f);
    //mTemperatureBuffer[pointIndex] = GameParameters::InitialTemperature;
    //mMaterialHeatCapacityBuffer[pointIndex] = structuralMaterial.GetHeatCapacity();
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = structuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 3.0f; // Debris are susceptible to wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::Debris;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxLifetime = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::DebrisState();

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor;

    // Remember that ephemeral points are dirty now
    mAreEphemeralPointsDirtyForRendering = true;
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
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mMaterialWaterVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    //mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    assert(mWaterBuffer[pointIndex] == 0.0f);
    //mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);
    //mIsLeakingBuffer[pointIndex] = false;

    assert(mTemperatureBuffer[pointIndex] == 0.0f);
    //mTemperatureBuffer[pointIndex] = GameParameters::InitialTemperature;
    //mMaterialHeatCapacityBuffer[pointIndex] = structuralMaterial.GetHeatCapacity();
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = structuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 20.0f; // Sparkles are susceptible to wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::Sparkle;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxLifetime = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::SparkleState();

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;
}

void Points::Detach(
    ElementIndex pointElementIndex,
    vec2f const & velocity,
    DetachOptions detachOptions,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Invoke ship detach handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandlePointDetach(
        pointElementIndex,
        !!(detachOptions & Points::DetachOptions::GenerateDebris),
        !!(detachOptions & Points::DetachOptions::FireDestroyEvent),
        currentSimulationTime,
        gameParameters);

    // Imprint velocity, unless the point is pinned
    if (!IsPinned(pointElementIndex))
    {
        mVelocityBuffer[pointElementIndex] = velocity;
    }
}

void Points::Restore(ElementIndex pointElementIndex)
{
    // Restore factory-time IsLeaking
    mIsLeakingBuffer[pointElementIndex] = mFactoryIsLeakingBuffer[pointElementIndex];

    // Restore combustion state
    mCombustionStateBuffer[pointElementIndex].Reset();
}

void Points::OnOrphaned(ElementIndex pointElementIndex)
{
    //
    // If we're in flames, make the flame tiny
    //

    if (mCombustionStateBuffer[pointElementIndex].State == CombustionState::StateType::Burning)
    {
        mCombustionStateBuffer[pointElementIndex].FlameDevelopment =
			0.1f + 0.04f * mRandomNormalizedUniformFloatBuffer[pointElementIndex];
    }
}

void Points::DestroyEphemeralParticle(
    ElementIndex pointElementIndex)
{
    // Invoke ship handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandleEphemeralParticleDestroy(pointElementIndex);

    // Fire destroy event
    mGameEventHandler->OnDestroy(
        GetStructuralMaterial(pointElementIndex),
        mParentWorld.IsUnderwater(GetPosition(pointElementIndex)),
        1u);

    // Expire particle
    ExpireEphemeralParticle(pointElementIndex);
}

void Points::UpdateForGameParameters(GameParameters const & gameParameters)
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
            mIntegrationFactorTimeCoefficientBuffer[i] = CalculateIntegrationFactorTimeCoefficient(
                numMechanicalDynamicsIterations,
                mFrozenCoefficientBuffer[i]);
        }

        // Remember the new values
        mCurrentNumMechanicalDynamicsIterations = numMechanicalDynamicsIterations;
    }

    float const cumulatedIntakenWaterThresholdForAirBubbles = gameParameters.CumulatedIntakenWaterThresholdForAirBubbles;
    if (cumulatedIntakenWaterThresholdForAirBubbles != mCurrentCumulatedIntakenWaterThresholdForAirBubbles)
    {
        // Randomize cumulated water intaken for each leaking point
        for (ElementIndex i : RawShipPoints())
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
    ElementIndex pointOffset,
    ElementIndex pointStride,
    float currentSimulationTime,
    float dt,
	Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    //
    // Take care of following:
    // - NotBurning->Developing transition (Ignition)
    // - Burning->Decay, Extinguishing transition
    //

    // Prepare candidates for ignition and explosion; we'll pick the top N ones
    // based on the ignition temperature delta
    mCombustionIgnitionCandidates.clear();
    mCombustionExplosionCandidates.clear();

    // Decay rate - the higher this value, the slower fire consumes materials
    float const effectiveCombustionDecayRate = (90.0f / (gameParameters.CombustionSpeedAdjustment * dt));

    // The cdf for rain: we stop burning with a probability equal to this
    float const rainExtinguishCdf = FastPow(stormParameters.RainDensity, 0.5f);

    // No real reason not to do ephemeral points as well, other than they're
    // currently not expected to burn
    for (ElementIndex pointIndex = pointOffset; pointIndex < mRawShipPointCount; pointIndex += pointStride)
    {
        auto const currentState = mCombustionStateBuffer[pointIndex].State;
        if (currentState == CombustionState::StateType::NotBurning)
        {
            //
            // See if this point should start burning
            //

            float const effectiveIgnitionTemperature =
                mMaterialIgnitionTemperatureBuffer[pointIndex] * gameParameters.IgnitionTemperatureAdjustment;

			// Note: we don't check for rain on purpose: we allow flames to develop even if it rains,
			// we'll eventually smother them later
            if (GetTemperature(pointIndex) >= effectiveIgnitionTemperature + GameParameters::IgnitionTemperatureHighWatermark
                && GetWater(pointIndex) < GameParameters::SmotheringWaterLowWatermark
                && GetDecay(pointIndex) > GameParameters::SmotheringDecayHighWatermark)
            {
                auto const combustionType = mMaterialCombustionTypeBuffer[pointIndex];

                if (combustionType == StructuralMaterial::MaterialCombustionType::Combustion
                    && !mParentWorld.IsUnderwater(GetPosition(pointIndex)))
                {
                    // Store point as ignition candidate
                    mCombustionIgnitionCandidates.emplace_back(
                        pointIndex,
                        (GetTemperature(pointIndex) - effectiveIgnitionTemperature) / effectiveIgnitionTemperature);
                }
                else if (combustionType == StructuralMaterial::MaterialCombustionType::Explosion)
                {
                    // Store point as explosion candidate
                    mCombustionExplosionCandidates.emplace_back(
                        pointIndex,
                        (GetTemperature(pointIndex) - effectiveIgnitionTemperature) / effectiveIgnitionTemperature);
                }
            }
        }
        else if (currentState == CombustionState::StateType::Burning)
        {
            //
            // See if this point should start extinguishing...
            //

            // ...for water or sea: we do this check at high frequency

            // ...for temperature or decay or rain: we check it here

            float const effectiveIgnitionTemperature =
                mMaterialIgnitionTemperatureBuffer[pointIndex] * gameParameters.IgnitionTemperatureAdjustment;

            if (GetTemperature(pointIndex) <= (effectiveIgnitionTemperature + GameParameters::IgnitionTemperatureLowWatermark)
                || GetDecay(pointIndex) < GameParameters::SmotheringDecayLowWatermark)
            {
                //
                // Transition to Extinguishing - by consumption
                //

                mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Extinguishing_Consumed;

                // Notify combustion end
                mGameEventHandler->OnPointCombustionEnd();
            }
			else if (GameRandomEngine::GetInstance().GenerateUniformBoolean(rainExtinguishCdf))
			{
				//
				// Transition to Extinguishing - by smothering for rain
				//

				SmotherCombustion(pointIndex, false);
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
                    0.15f); // Magic number: one tenth of the mass is 0.70 times the number of steps

                float const totalDecaySteps =
                    effectiveCombustionDecayRate
                    * massMultiplier;

                // decay(@ step i) = alpha^i
                // decay(@ step T) = min_decay => alpha^T = min_decay => alpha = min_decay^(1/T)
                float const decayAlpha = pow(0.01f, 1.0f / totalDecaySteps);

                // Decay point
                mDecayBuffer[pointIndex] *= decayAlpha;


                //
                // 2. Decay neighbors
                //

                for (auto const s : GetConnectedSprings(pointIndex).ConnectedSprings)
                {
                    mDecayBuffer[s.OtherEndpointIndex] *= decayAlpha;
                }
            }
        }
    }


    //
    // Pick candidates for ignition
    //

    // Randomly choose the max number of points we want to ignite now,
    // honoring MaxBurningParticles at the same time
    size_t const maxIgnitionPoints = std::min(
        std::min(
            size_t(4) + GameRandomEngine::GetInstance().Choose(size_t(6)), // 4->9
            mBurningPoints.size() < gameParameters.MaxBurningParticles
                ? static_cast<size_t>(gameParameters.MaxBurningParticles) - mBurningPoints.size()
                : size_t(0)),
        mCombustionIgnitionCandidates.size());

    // Sort top N candidates by ignition temperature delta
    std::nth_element(
        mCombustionIgnitionCandidates.data(),
        mCombustionIgnitionCandidates.data() + maxIgnitionPoints,
        mCombustionIgnitionCandidates.data() + mCombustionIgnitionCandidates.size(),
        [](auto const & t1, auto const & t2)
        {
            return std::get<1>(t1) > std::get<1>(t2);
        });

    // Ignite these points
    for (size_t i = 0; i < maxIgnitionPoints; ++i)
    {
        assert(i < mCombustionIgnitionCandidates.size());

        auto const pointIndex = std::get<0>(mCombustionIgnitionCandidates[i]);

        //
        // Ignite!
        //

        mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Developing_1;

        // Initial development depends on how deep this particle is in its burning zone
        mCombustionStateBuffer[pointIndex].FlameDevelopment =
            0.1f + 0.5f * SmoothStep(0.0f, 2.0f, std::get<1>(mCombustionIgnitionCandidates[i]));

        // Max development: random and depending on number of springs connected to this point
        // (so chains have smaller flames)
        float const deltaSizeDueToConnectedSprings =
            static_cast<float>(mConnectedSpringsBuffer[pointIndex].ConnectedSprings.size())
            * 0.0625f; // 0.0625 -> 0.50 (@8)
        mCombustionStateBuffer[pointIndex].MaxFlameDevelopment = std::max(
            0.25f + deltaSizeDueToConnectedSprings + 0.5f * mRandomNormalizedUniformFloatBuffer[pointIndex], // 0.25 + dsdtcs -> 0.75 + dsdtcs
            mCombustionStateBuffer[pointIndex].FlameDevelopment);

        // Add point to vector of burning points, sorted by plane ID
        assert(mBurningPoints.cend() == std::find(mBurningPoints.cbegin(), mBurningPoints.cend(), pointIndex));
        mBurningPoints.insert(
            std::lower_bound( // Earlier than others at same plane ID, so it's drawn behind them
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


    //
    // Pick candidates for explosion
    //

    // TODOTEST
    size_t const maxExplosionPoints = std::min(size_t(6), mCombustionExplosionCandidates.size());

    // Sort top N candidates by ignition temperature delta
    std::nth_element(
        mCombustionExplosionCandidates.data(),
        mCombustionExplosionCandidates.data() + maxExplosionPoints,
        mCombustionExplosionCandidates.data() + mCombustionExplosionCandidates.size(),
        [](auto const & t1, auto const & t2)
        {
            return std::get<1>(t1) > std::get<1>(t2);
        });

    // Explode these points
    for (size_t i = 0; i < maxExplosionPoints; ++i)
    {
        assert(i < mCombustionExplosionCandidates.size());

        auto const pointIndex = std::get<0>(mCombustionExplosionCandidates[i]);
        auto const pointPosition = GetPosition(pointIndex);

        //
        // Explode!
        //

        // Blast radius
        float const blastRadius =
            gameParameters.BombBlastRadius // TODOHERE: new GameParameter
            * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

        // Blast strength
        float const blastStrength =
            700.0f // Magic number
            * gameParameters.BombBlastForceAdjustment;

        // Blast heat
        float const blastHeat =
            gameParameters.BombBlastHeat // TODOHERE
            * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

        // Start explosion
        mShipPhysicsHandler->StartExplosion(
            currentSimulationTime,
            GetPlaneId(pointIndex),
            pointPosition,
            blastRadius,
            blastStrength,
            blastHeat,
            ExplosionType::Combustion,
            gameParameters);

        // Notify explosion
        mGameEventHandler->OnCombustionExplosion(
            mParentWorld.IsUnderwater(pointPosition),
            1);

        // Transition state
        mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Exploded;
    }
}

void Points::UpdateCombustionHighFrequency(
    float /*currentSimulationTime*/,
    float dt,
    GameParameters const & gameParameters)
{
    //
    // For all burning points, take care of following:
    // - Developing points: development up
    // - Burning points: heat generation
    // - Extinguishing points: development down
    //

    // Heat generated by combustion in this step
    float const effectiveCombustionHeat =
        100.0f * 1000.0f // 100KJ
        * dt
        * gameParameters.CombustionHeatAdjustment;

    for (auto const pointIndex : mBurningPoints)
    {
        //
        // Check if this point should stop developing/burning or start extinguishing faster
        //

        auto const currentState = mCombustionStateBuffer[pointIndex].State;

        if ((currentState == CombustionState::StateType::Developing_1
            || currentState == CombustionState::StateType::Developing_2
            || currentState == CombustionState::StateType::Burning
            || currentState == CombustionState::StateType::Extinguishing_Consumed)
            && (mParentWorld.IsUnderwater(GetPosition(pointIndex))
                || GetWater(pointIndex) > GameParameters::SmotheringWaterHighWatermark))
        {
            //
            // Transition to Extinguishing - by smothering for water
            //

            SmotherCombustion(pointIndex, true);
        }
        else if (currentState == CombustionState::StateType::Burning)
        {
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
                // 0.9 + 0.8*(1 - cos(theta)): 2.5 N, 0.9 S, 1.7 W and E
                vec2f const springDir = (GetPosition(otherEndpointIndex) - GetPosition(pointIndex)).normalise();
                float const dirAlpha =
                    (0.9f + 0.8f * (1.0f - springDir.dot(GameParameters::GravityNormalized)));
                // No normalization: when using normalization flame does not propagate along rope

                // Add heat to point
                mTemperatureBuffer[otherEndpointIndex] +=
                    effectiveCombustionHeat
                    * dirAlpha
                    / mMaterialHeatCapacityBuffer[otherEndpointIndex];
            }
        }


        //
        // Run development/extinguishing state machine now
        //

        switch (mCombustionStateBuffer[pointIndex].State)
        {
            case CombustionState::StateType::Developing_1:
            {
                //
                // Develop
                //
                // f(n-1) + 0.105*f(n-1): when starting from 0.1, after 25 steps (0.5s) it's 1.21
                // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:0.1,%22valid%22:true}]&expression=f(n-1)%20+%200.105*f(n-1)&target=0&endTarget=25&range=true
                //

                mCombustionStateBuffer[pointIndex].FlameDevelopment +=
                    0.105f * mCombustionStateBuffer[pointIndex].FlameDevelopment;

                // Check whether it's time to transition to the next development phase
                if (mCombustionStateBuffer[pointIndex].FlameDevelopment > mCombustionStateBuffer[pointIndex].MaxFlameDevelopment + 0.2f)
                {
                    mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Developing_2;
                }

                break;
            }

            case CombustionState::StateType::Developing_2:
            {
                //
                // Develop
                //
                // f(n-1) - 0.2*f(n-1): when starting from 0.2, after 10 steps (0.2s) it's below 0.02
                // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:0.2,%22valid%22:true}]&expression=f(n-1)%20-%200.2*f(n-1)&target=0&endTarget=25&range=true
                //

                // FlameDevelopment is now in the (MFD, MFD + 0.2) range
                auto extraFlameDevelopment = mCombustionStateBuffer[pointIndex].FlameDevelopment - mCombustionStateBuffer[pointIndex].MaxFlameDevelopment;
                extraFlameDevelopment =
                    extraFlameDevelopment
                    - 0.2f * extraFlameDevelopment;

                mCombustionStateBuffer[pointIndex].FlameDevelopment =
                    mCombustionStateBuffer[pointIndex].MaxFlameDevelopment + extraFlameDevelopment;

                // Check whether it's time to transition to burning
                if (extraFlameDevelopment < 0.02f)
                {
                    mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Burning;
                    mCombustionStateBuffer[pointIndex].FlameDevelopment = mCombustionStateBuffer[pointIndex].MaxFlameDevelopment;
                }

                break;
            }

            case CombustionState::StateType::Extinguishing_Consumed:
            case CombustionState::StateType::Extinguishing_SmotheredRain:
            case CombustionState::StateType::Extinguishing_SmotheredWater:
            {
                //
                // Un-develop
                //

                if (mCombustionStateBuffer[pointIndex].State == CombustionState::StateType::Extinguishing_Consumed)
                {
                    //
                    // f(n-1) - 0.0625*(1.01 - f(n-1)): when starting from 1, after 75 steps (1.5s) it's under 0.02
                    // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:1,%22valid%22:true}]&expression=f(n-1)%20-%200.0625*(1.01%20-%20f(n-1))&target=0&endTarget=80&range=true
                    //

                    mCombustionStateBuffer[pointIndex].FlameDevelopment -=
                        0.0625f
                        * (mCombustionStateBuffer[pointIndex].MaxFlameDevelopment - mCombustionStateBuffer[pointIndex].FlameDevelopment + 0.01f);
                }
                else if (mCombustionStateBuffer[pointIndex].State == CombustionState::StateType::Extinguishing_SmotheredRain)
                {
                    //
                    // f(n-1) - 0.075*f(n-1): when starting from 1, after 50 steps (1.0s) it's under 0.02
                    // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:1,%22valid%22:true}]&expression=f(n-1)%20-%200.075*f(n-1)&target=0&endTarget=75&range=true
                    //

                    mCombustionStateBuffer[pointIndex].FlameDevelopment -=
                        0.075f * mCombustionStateBuffer[pointIndex].FlameDevelopment;
                }
                else
                {
                    assert(mCombustionStateBuffer[pointIndex].State == CombustionState::StateType::Extinguishing_SmotheredWater);

                    //
                    // f(n-1) - 0.3*f(n-1): when starting from 1, after 10 steps (0.2s) it's under 0.02
                    // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:1,%22valid%22:true}]&expression=f(n-1)%20-%200.3*f(n-1)&target=0&endTarget=25&range=true
                    //

                    mCombustionStateBuffer[pointIndex].FlameDevelopment -=
                        0.3f * mCombustionStateBuffer[pointIndex].FlameDevelopment;
                }

                // Check whether we are done now
                if (mCombustionStateBuffer[pointIndex].FlameDevelopment <= 0.02f)
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

            case CombustionState::StateType::Burning:
            case CombustionState::StateType::NotBurning:
            case CombustionState::StateType::Exploded:
            {
                // Nothing to do here
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
                    if (!IsPinned(pointIndex))
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

                            auto & state = mEphemeralParticleAttributes2Buffer[pointIndex].State.AirBubble;

                            state.CurrentDeltaY = deltaY;
                            state.Progress =
                                -1.0f
                                / (-1.0f + std::min(GetPosition(pointIndex).y, 0.0f));

                            //
                            // Update vortex
                            //

                            float const lifetime =
                                currentSimulationTime
                                - mEphemeralParticleAttributes1Buffer[pointIndex].StartTime;

                            float const vortexAmplitude = state.VortexAmplitude + state.Progress;

                            float vortexValue =
                                vortexAmplitude
                                * PrecalcLoFreqSin.GetNearestPeriodic(state.NormalizedVortexAngularVelocity * lifetime);

                            // Update position with delta
                            mPositionBuffer[pointIndex].x += vortexValue - state.LastVortexValue;

                            state.LastVortexValue = vortexValue;
                        }
                    }

                    break;
                }

                case EphemeralType::Debris:
                {
                    // Check if expired
                    auto const elapsedLifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[pointIndex].StartTime;
                    auto const maxLifetime = mEphemeralParticleAttributes2Buffer[pointIndex].MaxLifetime;
                    if (elapsedLifetime >= maxLifetime)
                    {
                        ExpireEphemeralParticle(pointIndex);

                        // Remember that ephemeral points are now dirty
                        mAreEphemeralPointsDirtyForRendering = true;
                    }
                    else
                    {
                        // Update alpha based off remaining time

                        float alpha = std::max(
                            1.0f - elapsedLifetime / maxLifetime,
                            0.0f);

                        mColorBuffer[pointIndex].w = alpha;
                    }

                    break;
                }

                case EphemeralType::Sparkle:
                {
                    // Check if expired
                    auto const elapsedLifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[pointIndex].StartTime;
                    auto const maxLifetime = mEphemeralParticleAttributes2Buffer[pointIndex].MaxLifetime;
                    if (elapsedLifetime >= maxLifetime
                        || mParentWorld.IsUnderwater(GetPosition(pointIndex)))
                    {
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        // Update progress based off remaining time

                        mEphemeralParticleAttributes2Buffer[pointIndex].State.Sparkle.Progress =
                            elapsedLifetime / maxLifetime;
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
    LogMessage("W=", mWaterBuffer[pointElementIndex], " L=", mLightBuffer[pointElementIndex], " T=", mTemperatureBuffer[pointElementIndex], " Decay=", mDecayBuffer[pointElementIndex]);
    //LogMessage("Springs: ", mConnectedSpringsBuffer[pointElementIndex].ConnectedSprings.size(), " (factory: ", mFactoryConnectedSpringsBuffer[pointElementIndex].ConnectedSprings.size(), ")");
    LogMessage("PlaneID: ", mPlaneIdBuffer[pointElementIndex], " ConnectedComponentID: ", mConnectedComponentIdBuffer[pointElementIndex]);
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
            &(mColorBuffer.data()[mAlignedShipPointCount]),
            mAlignedShipPointCount,
            mEphemeralPointCount);
    }

    //
    // Upload mutable attributes
    //

    // We only upload all points for the first upload; for subsequent uploads,
    // depending on the buffer we only need to upload non-ephemeral points
    size_t const partialPointCount = mHaveWholeBuffersBeenUploadedOnce ? mRawShipPointCount : mAllPointCount;

    renderContext.UploadShipPointMutableAttributesStart(shipId);

    renderContext.UploadShipPointMutableAttributes(
        shipId,
        mPositionBuffer.data(),
        mLightBuffer.data(),
        mWaterBuffer.data(),
        partialPointCount);

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
                mRawShipPointCount);
        }

        mIsPlaneIdBufferNonEphemeralDirty = false;
    }
    else if (mIsPlaneIdBufferEphemeralDirty)
    {
        // Just ephemeral portion

        renderContext.UploadShipPointMutableAttributesPlaneId(
            shipId,
            &(mPlaneIdFloatBuffer.data()[mAlignedShipPointCount]),
            mAlignedShipPointCount,
            mEphemeralPointCount);

        mIsPlaneIdBufferEphemeralDirty = false;
    }

    if (mIsDecayBufferDirty)
    {
        renderContext.UploadShipPointMutableAttributesDecay(
            shipId,
            mDecayBuffer.data(),
            0,
            partialPointCount);

        mIsDecayBufferDirty = false;
    }

    if (renderContext.GetDrawHeatOverlay())
    {
        renderContext.UploadShipPointTemperature(
            shipId,
            mTemperatureBuffer.data(),
            0,
            partialPointCount);
    }

    renderContext.UploadShipPointMutableAttributesEnd(shipId);

    mHaveWholeBuffersBeenUploadedOnce = true;
}

void Points::UploadNonEphemeralPointElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    bool const doUploadAllPoints = (DebugShipRenderMode::Points == renderContext.GetDebugShipRenderMode());

    for (ElementIndex pointIndex : RawShipPoints())
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
        renderContext.UploadShipFlamesStart(shipId, mBurningPoints.size(), windSpeedMagnitude);

        // Upload flames, in order of plane ID
        for (auto const pointIndex : mBurningPoints)
        {
            renderContext.UploadShipFlame(
                shipId,
                GetPlaneId(pointIndex),
                GetPosition(pointIndex),
                mCombustionStateBuffer[pointIndex].FlameDevelopment,
                mRandomNormalizedUniformFloatBuffer[pointIndex],
                // IsOnChain: we use # of triangles as a heuristic for the point being on a chain,
                // and we use the *factory* ones to avoid sudden depth jumps when triangles are destroyed by fire
                mFactoryConnectedTrianglesBuffer[pointIndex].ConnectedTriangles.empty());
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

    if (mAreEphemeralPointsDirtyForRendering)
    {
        renderContext.UploadShipElementEphemeralPointsStart(shipId);
    }

    renderContext.UploadShipSparklesStart(shipId);

    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        switch (GetEphemeralType(pointIndex))
        {
            case EphemeralType::AirBubble:
            {
                auto const & state = mEphemeralParticleAttributes2Buffer[pointIndex].State.AirBubble;

                renderContext.UploadShipAirBubble(
                    shipId,
                    GetPlaneId(pointIndex),
                    GetPosition(pointIndex),
                    0.3f, // Scale, magic number
                    std::min(1.0f, state.CurrentDeltaY / 4.0f)); // Alpha

                break;
            }

            case EphemeralType::Debris:
            {
                // Don't upload point unless there's been a change
                if (mAreEphemeralPointsDirtyForRendering)
                {
                    renderContext.UploadShipElementEphemeralPoint(
                        shipId,
                        pointIndex);
                }

                break;
            }

            case EphemeralType::Sparkle:
            {
                vec2f const velocityVector =
					-GetVelocity(pointIndex)
					/ GameParameters::MaxSparkleParticlesForCutVelocity; // We use the cut sparkles arbitrarily

                renderContext.UploadShipSparkle(
                    shipId,
                    GetPlaneId(pointIndex),
                    GetPosition(pointIndex),
                    velocityVector,
                    mEphemeralParticleAttributes2Buffer[pointIndex].State.Sparkle.Progress);

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

    renderContext.UploadShipSparklesEnd(shipId);

    if (mAreEphemeralPointsDirtyForRendering)
    {
        renderContext.UploadShipElementEphemeralPointsEnd(shipId);

        // Not dirty anymore
        mAreEphemeralPointsDirtyForRendering = false;
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
        springs.UpdateForMass(connectedSpring.SpringIndex, *this);
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

    assert(mFreeEphemeralParticleSearchStartIndex >= mAlignedShipPointCount
        && mFreeEphemeralParticleSearchStartIndex < mAllPointCount);

    for (ElementIndex p = mFreeEphemeralParticleSearchStartIndex; ; /*incremented in loop*/)
    {
        if (EphemeralType::None == mEphemeralParticleAttributes1Buffer[p].Type)
        {
            // Found!

            // Remember to start after this one next time
            mFreeEphemeralParticleSearchStartIndex = p + 1;
            if (mFreeEphemeralParticleSearchStartIndex >= mAllPointCount)
                mFreeEphemeralParticleSearchStartIndex = mAlignedShipPointCount;

            return p;
        }

        // Check whether it's the oldest
        auto lifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[p].StartTime;
        if (lifetime >= oldestParticleLifetime)
        {
            oldestParticle = p;
            oldestParticleLifetime = lifetime;
        }

        // Advance
        ++p;
        if (p >= mAllPointCount)
            p = mAlignedShipPointCount;

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
        mFreeEphemeralParticleSearchStartIndex = mAlignedShipPointCount;

    return oldestParticle;
}

}