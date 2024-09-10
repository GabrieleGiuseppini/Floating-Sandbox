/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/Log.h>
#include <GameCore/PrecalculatedFunction.h>

#include <cmath>
#include <limits>

namespace Physics {

void Points::Add(
    vec2f const & position,
    float water,
    float internalPressure,
    StructuralMaterial const & structuralMaterial,
    ElectricalMaterial const * electricalMaterial,
    bool isRope,
    float strength,
    ElementIndex electricalElementIndex,
    bool isStructurallyLeaking,
    rgbaColor const & color,
    vec2f const & textureCoordinates,
    float randomNormalizedUniformFloat)
{
    ElementIndex const pointIndex = static_cast<ElementIndex>(mIsDamagedBuffer.GetCurrentPopulatedSize());

    mIsDamagedBuffer.emplace_back(false);
    mMaterialsBuffer.emplace_back(&structuralMaterial, electricalMaterial);
    mIsRopeBuffer.emplace_back(isRope);

    mPositionBuffer.emplace_back(position);
    mFactoryPositionBuffer.emplace_back(position);
    mVelocityBuffer.emplace_back(vec2f::zero());
    // First buffer implicitly
    assert(mDynamicForceBuffers.size() >= 1);
    mDynamicForceBuffers[0].emplace_back(vec2f::zero());
    mStaticForceBuffer.emplace_back(vec2f::zero());
    mAugmentedMaterialMassBuffer.emplace_back(structuralMaterial.GetMass());
    mTransientAdditionalMassBuffer.emplace_back(0.0f);
    mMassBuffer.emplace_back(structuralMaterial.GetMass());
    mMaterialBuoyancyVolumeFillBuffer.emplace_back(structuralMaterial.BuoyancyVolumeFill);
    mStrengthBuffer.emplace_back(strength);
    mStressBuffer.emplace_back(0.0f);
    mDecayBuffer.emplace_back(1.0f);
    mFrozenCoefficientBuffer.emplace_back(1.0f);
    mIntegrationFactorTimeCoefficientBuffer.emplace_back(CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f));
    mBuoyancyCoefficientsBuffer.emplace_back(CalculateBuoyancyCoefficients(
        structuralMaterial.BuoyancyVolumeFill,
        structuralMaterial.ThermalExpansionCoefficient));
    mOceanFloorCollisionFactorsBuffer.emplace_back(CalculateOceanFloorCollisionFactors(
        mCurrentElasticityAdjustment,
        mCurrentStaticFrictionAdjustment,
        mCurrentKineticFrictionAdjustment,
        mCurrentOceanFloorElasticityCoefficient,
        mCurrentOceanFloorFrictionCoefficient,
        structuralMaterial.ElasticityCoefficient,
        structuralMaterial.StaticFrictionCoefficient,
        structuralMaterial.KineticFrictionCoefficient));
    mCachedDepthBuffer.emplace_back(mParentWorld.GetOceanSurface().GetDepth(position));

    mIntegrationFactorBuffer.emplace_back(vec2f::zero());

    mInternalPressureBuffer.emplace_back(internalPressure);
    mIsHullBuffer.emplace_back(structuralMaterial.IsHull); // Default is from material
    mMaterialWaterIntakeBuffer.emplace_back(structuralMaterial.WaterIntake);
    mMaterialWaterRestitutionBuffer.emplace_back(1.0f - structuralMaterial.WaterRetention);
    mMaterialWaterDiffusionSpeedBuffer.emplace_back(structuralMaterial.WaterDiffusionSpeed);

    mWaterBuffer.emplace_back(water);
    mWaterVelocityBuffer.emplace_back(vec2f::zero());
    mWaterMomentumBuffer.emplace_back(vec2f::zero());
    mCumulatedIntakenWater.emplace_back(0.0f);
    mLeakingCompositeBuffer.emplace_back(LeakingComposite(isStructurallyLeaking));
    if (isStructurallyLeaking)
        SetStructurallyLeaking(pointIndex);
    mFactoryIsStructurallyLeakingBuffer.emplace_back(isStructurallyLeaking);
    mTotalFactoryWetPoints += (water > 0.0f ? 1 : 0);

    // Heat dynamics
    mTemperatureBuffer.emplace_back(GameParameters::Temperature0);
    assert(structuralMaterial.GetHeatCapacity() > 0.0f);
    mMaterialHeatCapacityReciprocalBuffer.emplace_back(1.0f / structuralMaterial.GetHeatCapacity());
    mMaterialThermalExpansionCoefficientBuffer.emplace_back(structuralMaterial.ThermalExpansionCoefficient);
    mMaterialIgnitionTemperatureBuffer.emplace_back(structuralMaterial.IgnitionTemperature);
    mMaterialCombustionTypeBuffer.emplace_back(structuralMaterial.CombustionType);
    mCombustionStateBuffer.emplace_back(CombustionState());

    // Water raction dynamics
    mWaterReactionStateBuffer.emplace_back(structuralMaterial.WaterReactivity);

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

    // Gadgets
    mIsGadgetAttachedBuffer.emplace_back(false);

    // Randomness
    mRandomNormalizedUniformFloatBuffer.emplace_back(randomNormalizedUniformFloat);

    // Immutable render attributes
    mColorBuffer.emplace_back(color.toVec4f());
    mTextureCoordinatesBuffer.emplace_back(textureCoordinates);
}

void Points::CreateEphemeralParticleAirBubble(
    vec2f const & position,
    float depth,
    float temperature,
    float buoyancyVolumeFillAdjustment,
    float vortexAmplitude,
    float vortexPeriod,
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

    StructuralMaterial const & airStructuralMaterial = mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Air);

    // We want to limit the buoyancy applied to air - using 1.0 makes an air particle boost up too quickly;
    // after all, bubbles encounter a lot of drag...
    float const airBubbleBuoyancyVolumeFill = 0.002f * buoyancyVolumeFillAdjustment;

    assert(mIsDamagedBuffer[pointIndex] == false); // Ephemeral points are never damaged
    mMaterialsBuffer[pointIndex] = Materials(&airStructuralMaterial, nullptr);
    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = vec2f::zero();
    assert(mDynamicForceBuffers[0][pointIndex] == vec2f::zero()); // Ephemeral points never participate in dynamic forces (springs + surface pressure)
    mStaticForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = airStructuralMaterial.GetMass();
    mTransientAdditionalMassBuffer[pointIndex] = 0.0f;
    mMassBuffer[pointIndex] = airStructuralMaterial.GetMass();
    mMaterialBuoyancyVolumeFillBuffer[pointIndex] = airBubbleBuoyancyVolumeFill;
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mBuoyancyCoefficientsBuffer[pointIndex] = CalculateBuoyancyCoefficients(
        airBubbleBuoyancyVolumeFill,
        airStructuralMaterial.ThermalExpansionCoefficient);
    mOceanFloorCollisionFactorsBuffer[pointIndex] = CalculateOceanFloorCollisionFactors(
        mCurrentElasticityAdjustment,
        mCurrentStaticFrictionAdjustment,
        mCurrentKineticFrictionAdjustment,
        mCurrentOceanFloorElasticityCoefficient,
        mCurrentOceanFloorFrictionCoefficient,
        airStructuralMaterial.ElasticityCoefficient,
        airStructuralMaterial.StaticFrictionCoefficient,
        airStructuralMaterial.KineticFrictionCoefficient);
    mCachedDepthBuffer[pointIndex] = depth;

    //mInternalPressureBuffer[pointIndex] = 0.0f; // There's no hull hence we won't need it
    //mMaterialWaterIntakeBuffer[pointIndex] = airStructuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - airStructuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = airStructuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(!mLeakingCompositeBuffer[pointIndex].IsCumulativelyLeaking);
    //mLeakingCompositeBuffer[pointIndex] = LeakingComposite(false);

    mTemperatureBuffer[pointIndex] = temperature;
    assert(airStructuralMaterial.GetHeatCapacity() > 0.0f);
    mMaterialHeatCapacityReciprocalBuffer[pointIndex] = 1.0f / airStructuralMaterial.GetHeatCapacity();
    mMaterialThermalExpansionCoefficientBuffer[pointIndex] = airStructuralMaterial.ThermalExpansionCoefficient;
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = airStructuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = airStructuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    //mWaterReactionStateBuffer.emplace_back(0.0f);

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 0.0f; // Air bubbles (underwater) do not care about wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::AirBubble;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime = std::numeric_limits<float>::max();
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::AirBubbleState(
        vortexAmplitude,
        vortexPeriod);

    assert(mConnectedComponentIdBuffer[pointIndex] == NoneConnectedComponentId);
    //mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    mColorBuffer[pointIndex] = airStructuralMaterial.RenderColor.toVec4f();
    mIsEphemeralColorBufferDirty = true;
}

void Points::CreateEphemeralParticleDebris(
    vec2f const & position,
    vec2f const & velocity,
    float depth,
    float water,
    StructuralMaterial const & structuralMaterial,
    float currentSimulationTime,
    float maxSimulationLifetime,
    PlaneId planeId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, true);
    assert(NoneElementIndex != pointIndex);

    //
    // Store attributes
    //

    assert(mIsDamagedBuffer[pointIndex] == false); // Ephemeral points are never damaged
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);
    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    assert(mDynamicForceBuffers[0][pointIndex] == vec2f::zero()); // Ephemeral points never participate in springs + surface pressure
    mStaticForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mTransientAdditionalMassBuffer[pointIndex] = 0.0f;
    mMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mMaterialBuoyancyVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mBuoyancyCoefficientsBuffer[pointIndex] = BuoyancyCoefficients(0.0f, 0.0f); // No buoyancy
    mCachedDepthBuffer[pointIndex] = depth;

    //mInternalPressureBuffer[pointIndex] = 0.0f; // There's no hull hence we won't need it
    //mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = water;
    assert(!mLeakingCompositeBuffer[pointIndex].IsCumulativelyLeaking);
    //mLeakingCompositeBuffer[pointIndex] = LeakingComposite(false);

    mTemperatureBuffer[pointIndex] = GameParameters::Temperature0;
    assert(structuralMaterial.GetHeatCapacity() > 0.0f);
    mMaterialHeatCapacityReciprocalBuffer[pointIndex] = 1.0f / structuralMaterial.GetHeatCapacity();
    //mMaterialThermalExpansionCoefficientBuffer[pointIndex] = structuralMaterial.ThermalExpansionCoefficient;
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = structuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    //mWaterReactionStateBuffer.emplace_back(0.0f);

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 3.0f; // Debris is susceptible to wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::Debris;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime = maxSimulationLifetime;
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::DebrisState();

    assert(mConnectedComponentIdBuffer[pointIndex] == NoneConnectedComponentId);
    //mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor.toVec4f();
    mIsEphemeralColorBufferDirty = true;

    // Remember that ephemeral point elements are dirty now
    mAreEphemeralPointElementsDirtyForRendering = true;
}

void Points::CreateEphemeralParticleSmoke(
    Render::GenericMipMappedTextureGroups textureGroup,
    EphemeralState::SmokeState::GrowthType growth,
    vec2f const & position,
    float depth,
    float temperature,
    float currentSimulationTime,
    PlaneId planeId,
    GameParameters const & gameParameters)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, true);
    assert(NoneElementIndex != pointIndex);

    // Choose a lifetime
    float const maxSimulationLifetime =
        gameParameters.SmokeParticleLifetimeAdjustment
        * GameRandomEngine::GetInstance().GenerateUniformReal(
            GameParameters::MinSmokeParticlesLifetime,
            GameParameters::MaxSmokeParticlesLifetime);

    //
    // Store attributes
    //

    StructuralMaterial const & airStructuralMaterial = mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Air);

    assert(mIsDamagedBuffer[pointIndex] == false); // Ephemeral points are never damaged
    mMaterialsBuffer[pointIndex] = Materials(&airStructuralMaterial, nullptr);
    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = vec2f::zero();
    assert(mDynamicForceBuffers[0][pointIndex] == vec2f::zero()); // Ephemeral points never participate in springs nor surface pressure
    mStaticForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = airStructuralMaterial.GetMass();
    mTransientAdditionalMassBuffer[pointIndex] = 0.0f;
    mMassBuffer[pointIndex] = airStructuralMaterial.GetMass();
    mMaterialBuoyancyVolumeFillBuffer[pointIndex] = airStructuralMaterial.BuoyancyVolumeFill;
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mBuoyancyCoefficientsBuffer[pointIndex] = CalculateBuoyancyCoefficients(
        airStructuralMaterial.BuoyancyVolumeFill,
        airStructuralMaterial.ThermalExpansionCoefficient);
    mOceanFloorCollisionFactorsBuffer[pointIndex] = CalculateOceanFloorCollisionFactors(
        mCurrentElasticityAdjustment,
        mCurrentStaticFrictionAdjustment,
        mCurrentKineticFrictionAdjustment,
        mCurrentOceanFloorElasticityCoefficient,
        mCurrentOceanFloorFrictionCoefficient,
        airStructuralMaterial.ElasticityCoefficient,
        airStructuralMaterial.StaticFrictionCoefficient,
        airStructuralMaterial.KineticFrictionCoefficient);
    mCachedDepthBuffer[pointIndex] = depth;

    //mInternalPressureBuffer[pointIndex] = 0.0f; // There's no hull hence we won't need it
    //mMaterialWaterIntakeBuffer[pointIndex] = airStructuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - airStructuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = airStructuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(!mLeakingCompositeBuffer[pointIndex].IsCumulativelyLeaking);
    //mLeakingCompositeBuffer[pointIndex] = LeakingComposite(false);

    mTemperatureBuffer[pointIndex] = temperature;
    assert(airStructuralMaterial.GetHeatCapacity() > 0.0f);
    mMaterialHeatCapacityReciprocalBuffer[pointIndex] = 1.0f / airStructuralMaterial.GetHeatCapacity();
    mMaterialThermalExpansionCoefficientBuffer[pointIndex] = airStructuralMaterial.ThermalExpansionCoefficient;
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = airStructuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = airStructuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    //mWaterReactionStateBuffer.emplace_back(0.0f);

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 0.2f; // Smoke cares about wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::Smoke;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime = maxSimulationLifetime;
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::SmokeState(
        textureGroup,
        growth,
        GameRandomEngine::GetInstance().GenerateNormalizedUniformReal());

    assert(mConnectedComponentIdBuffer[pointIndex] == NoneConnectedComponentId);
    //mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    mColorBuffer[pointIndex] = airStructuralMaterial.RenderColor.toVec4f();
    mIsEphemeralColorBufferDirty = true;
}

void Points::CreateEphemeralParticleSparkle(
    vec2f const & position,
    vec2f const & velocity,
    StructuralMaterial const & structuralMaterial,
    float depth,
    float currentSimulationTime,
    float maxSimulationLifetime,
    PlaneId planeId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, true);
    assert(NoneElementIndex != pointIndex);

    //
    // Store attributes
    //

    assert(mIsDamagedBuffer[pointIndex] == false); // Ephemeral points are never damaged
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);
    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    assert(mDynamicForceBuffers[0][pointIndex] == vec2f::zero()); // Ephemeral points never participate in springs + surface pressure
    mStaticForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mTransientAdditionalMassBuffer[pointIndex] = 0.0f;
    mMassBuffer[pointIndex] = structuralMaterial.GetMass();
    mMaterialBuoyancyVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mBuoyancyCoefficientsBuffer[pointIndex] = BuoyancyCoefficients(0.0f, 0.0f); // No buoyancy
    mCachedDepthBuffer[pointIndex] = depth;

    //mInternalPressureBuffer[pointIndex] = 0.0f; // There's no hull hence we won't need it
    //mMaterialWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(!mLeakingCompositeBuffer[pointIndex].IsCumulativelyLeaking);
    //mLeakingCompositeBuffer[pointIndex] = LeakingComposite(false);

    mTemperatureBuffer[pointIndex] = GameParameters::Temperature0;
    assert(structuralMaterial.GetHeatCapacity() > 0.0f);
    mMaterialHeatCapacityReciprocalBuffer[pointIndex] = 1.0f / structuralMaterial.GetHeatCapacity();
    //mMaterialThermalExpansionCoefficientBuffer[pointIndex] = structuralMaterial.ThermalExpansionCoefficient;
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = structuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = structuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    //mWaterReactionStateBuffer.emplace_back(0.0f);

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 20.0f; // Sparkles are susceptible to wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::Sparkle;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime = maxSimulationLifetime;
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::SparkleState();

    assert(mConnectedComponentIdBuffer[pointIndex] == NoneConnectedComponentId);
    //mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;
}

void Points::CreateEphemeralParticleWakeBubble(
    vec2f const & position,
    vec2f const & velocity,
    float depth,
    float currentSimulationTime,
    PlaneId planeId,
    GameParameters const & gameParameters)
{
    // Get a free slot (but don't steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, false);
    if (NoneElementIndex == pointIndex)
        return; // No luck

    //
    // Store attributes
    //

    StructuralMaterial const & waterStructuralMaterial = mMaterialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Water);

    assert(mIsDamagedBuffer[pointIndex] == false); // Ephemeral points are never damaged
    mMaterialsBuffer[pointIndex] = Materials(&waterStructuralMaterial, nullptr);
    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    assert(mDynamicForceBuffers[0][pointIndex] == vec2f::zero()); // Ephemeral points never participate in springs + surface pressure
    mStaticForceBuffer[pointIndex] = vec2f::zero();
    mAugmentedMaterialMassBuffer[pointIndex] = waterStructuralMaterial.GetMass();
    mTransientAdditionalMassBuffer[pointIndex] = 0.0f;
    mMassBuffer[pointIndex] = waterStructuralMaterial.GetMass();
    mMaterialBuoyancyVolumeFillBuffer[pointIndex] = waterStructuralMaterial.BuoyancyVolumeFill;
    assert(mDecayBuffer[pointIndex] == 1.0f);
    //mDecayBuffer[pointIndex] = 1.0f;
    mFrozenCoefficientBuffer[pointIndex] = 1.0f;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations, 1.0f);
    mBuoyancyCoefficientsBuffer[pointIndex] = CalculateBuoyancyCoefficients(
        waterStructuralMaterial.BuoyancyVolumeFill,
        waterStructuralMaterial.ThermalExpansionCoefficient);
    mOceanFloorCollisionFactorsBuffer[pointIndex] = CalculateOceanFloorCollisionFactors(
        mCurrentElasticityAdjustment,
        mCurrentStaticFrictionAdjustment,
        mCurrentKineticFrictionAdjustment,
        mCurrentOceanFloorElasticityCoefficient,
        mCurrentOceanFloorFrictionCoefficient,
        waterStructuralMaterial.ElasticityCoefficient,
        waterStructuralMaterial.StaticFrictionCoefficient,
        waterStructuralMaterial.KineticFrictionCoefficient);
    mCachedDepthBuffer[pointIndex] = depth;

    //mInternalPressureBuffer[pointIndex] = 0.0f; // There's no hull hence we won't need it
    //mMaterialWaterIntakeBuffer[pointIndex] = waterStructuralMaterial.WaterIntake;
    //mMaterialWaterRestitutionBuffer[pointIndex] = 1.0f - waterStructuralMaterial.WaterRetention;
    //mMaterialWaterDiffusionSpeedBuffer[pointIndex] = waterStructuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(!mLeakingCompositeBuffer[pointIndex].IsCumulativelyLeaking);
    //mLeakingCompositeBuffer[pointIndex] = LeakingComposite(false);

    mTemperatureBuffer[pointIndex] = gameParameters.WaterTemperature;
    assert(waterStructuralMaterial.GetHeatCapacity() > 0.0f);
    mMaterialHeatCapacityReciprocalBuffer[pointIndex] = 1.0f / waterStructuralMaterial.GetHeatCapacity();
    mMaterialThermalExpansionCoefficientBuffer[pointIndex] = waterStructuralMaterial.ThermalExpansionCoefficient;
    //mMaterialIgnitionTemperatureBuffer[pointIndex] = waterStructuralMaterial.IgnitionTemperature;
    //mMaterialCombustionTypeBuffer[pointIndex] = waterStructuralMaterial.CombustionType;
    //mCombustionStateBuffer[pointIndex] = CombustionState();

    //mWaterReactionStateBuffer.emplace_back(0.0f);

    assert(mLightBuffer[pointIndex] == 0.0f);
    //mLightBuffer[pointIndex] = 0.0f;

    mMaterialWindReceptivityBuffer[pointIndex] = 0.0f; // Wake bubbles (underwater) do not care about wind

    assert(mMaterialRustReceptivityBuffer[pointIndex] == 0.0f);
    //mMaterialRustReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralParticleAttributes1Buffer[pointIndex].Type = EphemeralType::WakeBubble;
    mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime = currentSimulationTime;
    mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime = 0.4f; // Magic number
    mEphemeralParticleAttributes2Buffer[pointIndex].State = EphemeralState::WakeBubbleState();

    assert(mConnectedComponentIdBuffer[pointIndex] == NoneConnectedComponentId);
    //mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    mColorBuffer[pointIndex] = waterStructuralMaterial.RenderColor.toVec4f();
    mIsEphemeralColorBufferDirty = true;
}

void Points::Detach(
    ElementIndex pointElementIndex,
    vec2f const & velocity,
    DetachOptions detachOptions,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // We don't detach ephemeral points
    assert(pointElementIndex < mAlignedShipPointCount);

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

    // Check if it's the first time we get damaged
    if (!mIsDamagedBuffer[pointElementIndex])
    {
        // Invoke handler
        mShipPhysicsHandler->HandlePointDamaged(pointElementIndex);

        // Flag ourselves as damaged
        mIsDamagedBuffer[pointElementIndex] = true;
    }
}

void Points::Restore(ElementIndex pointElementIndex)
{
    assert(IsDamaged(pointElementIndex));

    // Clear the damaged flag
    mIsDamagedBuffer[pointElementIndex] = false;

    // Restore factory-time structural IsLeaking
    mLeakingCompositeBuffer[pointElementIndex].LeakingSources.StructuralLeak =
        mFactoryIsStructurallyLeakingBuffer[pointElementIndex] ? 1.0f : 0.0f;

    // Remove point from set of burning points, in case it was burning
    if (mCombustionStateBuffer[pointElementIndex].State != CombustionState::StateType::NotBurning)
    {
        auto pointIt = std::find(
            mBurningPoints.cbegin(),
            mBurningPoints.cend(),
            pointElementIndex);
        assert(pointIt != mBurningPoints.cend());
        mBurningPoints.erase(pointIt);

        // Restore combustion state
        mCombustionStateBuffer[pointElementIndex].Reset();
    }

    // Reset water reaction state
    mWaterReactionStateBuffer[pointElementIndex].Reset();

    // Invoke ship handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandlePointRestore(pointElementIndex);
}

void Points::OnOrphaned(ElementIndex pointElementIndex)
{
    //
    // If we're in flames, make the flame tiny
    //

    if (mCombustionStateBuffer[pointElementIndex].State == CombustionState::StateType::Burning)
    {
        // New target: fraction of current size plus something
        mCombustionStateBuffer[pointElementIndex].MaxFlameDevelopment =
            mCombustionStateBuffer[pointElementIndex].FlameDevelopment / 3.0f
            + 0.04f * mRandomNormalizedUniformFloatBuffer[pointElementIndex];

        mCombustionStateBuffer[pointElementIndex].State = CombustionState::StateType::Developing_2;
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
        mParentWorld.GetOceanSurface().IsUnderwater(GetPosition(pointElementIndex)),
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

        // Remember the new value
        mCurrentNumMechanicalDynamicsIterations = numMechanicalDynamicsIterations;
    }

    float const elasticityAdjustment = gameParameters.ElasticityAdjustment;
    float const staticFrictionAdjustment = gameParameters.StaticFrictionAdjustment;
    float const kineticFrictionAdjustment = gameParameters.KineticFrictionAdjustment;
    float const oceanFloorElasticityCoefficient = gameParameters.OceanFloorElasticityCoefficient;
    float const oceanFloorFrictionCoefficient = gameParameters.OceanFloorFrictionCoefficient;
    if (elasticityAdjustment != mCurrentElasticityAdjustment
        || staticFrictionAdjustment != mCurrentStaticFrictionAdjustment
        || kineticFrictionAdjustment != mCurrentKineticFrictionAdjustment
        || oceanFloorElasticityCoefficient != mCurrentOceanFloorElasticityCoefficient
        || oceanFloorFrictionCoefficient != mCurrentOceanFloorFrictionCoefficient)
    {
        // Recalc ocean floor coefficients
        for (ElementIndex i : *this)
        {
            if (mMaterialsBuffer[i].Structural != nullptr)
            {
                mOceanFloorCollisionFactorsBuffer[i] = CalculateOceanFloorCollisionFactors(
                    elasticityAdjustment,
                    staticFrictionAdjustment,
                    kineticFrictionAdjustment,
                    oceanFloorElasticityCoefficient,
                    oceanFloorFrictionCoefficient,
                    mMaterialsBuffer[i].Structural->ElasticityCoefficient,
                    mMaterialsBuffer[i].Structural->StaticFrictionCoefficient,
                    mMaterialsBuffer[i].Structural->KineticFrictionCoefficient);
            }
        }

        // Remember the new values
        mCurrentOceanFloorElasticityCoefficient = oceanFloorElasticityCoefficient;
        mCurrentOceanFloorFrictionCoefficient = oceanFloorFrictionCoefficient;
        mCurrentElasticityAdjustment = elasticityAdjustment;
        mCurrentStaticFrictionAdjustment = staticFrictionAdjustment;
        mCurrentKineticFrictionAdjustment = kineticFrictionAdjustment;
    }

    float const cumulatedIntakenWaterThresholdForAirBubbles = GameParameters::AirBubblesDensityToCumulatedIntakenWater(gameParameters.AirBubblesDensity);
    if (cumulatedIntakenWaterThresholdForAirBubbles != mCurrentCumulatedIntakenWaterThresholdForAirBubbles)
    {
        // Randomize cumulated water intaken for each leaking point
        for (ElementIndex i : RawShipPoints())
        {
            if (GetLeakingComposite(i).IsCumulativelyLeaking)
            {
                mCumulatedIntakenWater[i] = RandomizeCumulatedIntakenWater(cumulatedIntakenWaterThresholdForAirBubbles);
            }
        }

        // Remember the new value
        mCurrentCumulatedIntakenWaterThresholdForAirBubbles = cumulatedIntakenWaterThresholdForAirBubbles;
    }

    float const combustionSpeedAdjustment = gameParameters.CombustionSpeedAdjustment;
    if (combustionSpeedAdjustment != mCurrentCombustionSpeedAdjustment)
    {
        // Recalc combustion decay parameters
        CalculateCombustionDecayParameters(combustionSpeedAdjustment, GameParameters::GameParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>);

        // Remember the new value
        mCurrentCombustionSpeedAdjustment = combustionSpeedAdjustment;
    }
}

void Points::UpdateCombustionLowFrequency(
    ElementIndex pointOffset,
    ElementIndex pointStride,
    GameWallClock::float_time currentWallClockTime,
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    /////////////////////////////////////////////////////////////////////////////
    // Take care of following:
    // - Combustion:
    //      - NotBurning->Developing transition (Ignition)
    //      - Burning->Decay, Extinguishing transition
    // - Water reactivity:
    //      - All transitions
    /////////////////////////////////////////////////////////////////////////////

    // Prepare candidate buffer; we'll pick the top N ones
    // based on "priority" (e.g. the ignition temperature delta)
    mCombustionIgnitionCandidates.clear();
    mCombustionExplosionCandidates.clear();
    mWaterReactionExplosionCandidates.clear();

    // The cdf for rain: we stop burning during this call with a probability equal to this
    float const rainExtinguishCdf = FastPow(stormParameters.RainDensity / 2.0f, 3.3f);

    //
    // Visit all points
    //
    // No real reason not to do ephemeral points as well, other than they're
    // currently not expected to burn
    //

    for (ElementIndex pointIndex = pointOffset; pointIndex < mRawShipPointCount; pointIndex += pointStride)
    {
        //
        // Combustion
        //

        auto const currentCombustionState = mCombustionStateBuffer[pointIndex].State;

        if (currentCombustionState == CombustionState::StateType::NotBurning)
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
                    && !IsCachedUnderwater(pointIndex))
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
        else if (currentCombustionState == CombustionState::StateType::Burning)
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

                // Lower heat or we'll start burning again (the trigger condition for smothering here
                // is merely the presence of rain, not the temperature)
                mTemperatureBuffer[pointIndex] = effectiveIgnitionTemperature + 1.5f * GameParameters::IgnitionTemperatureLowWatermark;
            }
            else
            {
                // Apply low-frequency effects of burning

                //
                // 1. Decay burning point
                //

                auto const pointMass = mMaterialsBuffer[pointIndex].Structural->GetMass();

                float const decayAlpha =
                    (mCombustionDecayAlphaFunctionA * pointMass + mCombustionDecayAlphaFunctionB) * pointMass
                    + mCombustionDecayAlphaFunctionC;

                assert(decayAlpha <= 1.0f); // We can't allow decay to grow

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

        //
        // Water reactivity
        //

        auto & waterReactivityState = mWaterReactionStateBuffer[pointIndex];

        if (waterReactivityState.State == WaterReactionState::StateType::Unreacted)
        {
            // See if should react
            float const waterReactionThreshold = 0.5f * waterReactivityState.Reactivity;
            if (GetWater(pointIndex) >= waterReactionThreshold)
            {
                // Switch state
                waterReactivityState.State = WaterReactionState::StateType::ReactionTriggered;
                waterReactivityState.ExplosionTimestamp = currentWallClockTime + 2.0f;

                // Notify water reaction
                mGameEventHandler->OnWaterReaction(
                    IsCachedUnderwater(pointIndex),
                    1);
            }
        }
        else if (waterReactivityState.State == WaterReactionState::StateType::ReactionTriggered)
        {
            // See if should explode
            if (currentWallClockTime >= waterReactivityState.ExplosionTimestamp)
            {
                // Store point as explosion candidate
                mWaterReactionExplosionCandidates.emplace_back(
                    pointIndex,
                    currentWallClockTime - waterReactivityState.ExplosionTimestamp);
            }
        }
    }

    //
    // Pick candidates for ignition
    //

    if (!mCombustionIgnitionCandidates.empty())
    {
        // Randomly choose the max number of points we want to ignite now,
        // honoring MaxBurningParticles at the same time
        size_t const maxIgnitionPoints = std::min(
            std::min(
                size_t(4) + GameRandomEngine::GetInstance().Choose(size_t(6)), // 4->9
                mBurningPoints.size() < gameParameters.MaxBurningParticlesPerShip
                ? static_cast<size_t>(gameParameters.MaxBurningParticlesPerShip) - mBurningPoints.size()
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

            // Calculate max development: random and depending on number of springs connected to this point
            // (so chains have smaller flames)
            float const deltaSizeDueToConnectedSprings =
                static_cast<float>(mConnectedSpringsBuffer[pointIndex].ConnectedSprings.size())
                * 0.0625f; // 0.0625 -> 0.50 (@8)
            mCombustionStateBuffer[pointIndex].MaxFlameDevelopment = std::max(
                0.25f + deltaSizeDueToConnectedSprings + 0.5f * mRandomNormalizedUniformFloatBuffer[pointIndex], // 0.25 + dsdtcs -> 0.75 + dsdtcs
                mCombustionStateBuffer[pointIndex].FlameDevelopment);

            // Initialize flame vector
            mCombustionStateBuffer[pointIndex].FlameVector = Formulae::CalculateIdealFlameVector(
                GetVelocity(pointIndex),
                200.0f); // For an initial flame, we want the particle's current velocity to have a smaller impact on the flame vector

            // Add point to vector of burning points, sorted by plane ID
            assert(mBurningPoints.cend() == std::find(mBurningPoints.cbegin(), mBurningPoints.cend(), pointIndex));
            mBurningPoints.insert(
                std::lower_bound( // Earlier than others at same plane ID, so it's drawn behind them
                    mBurningPoints.cbegin(),
                    mBurningPoints.cend(),
                    pointIndex,
                    [this](auto p1, auto p2)
                    {
                        // Sort by plane and then by vertical position, so bottommost flames cover uppermost ones
                        return mPlaneIdBuffer[p1] < mPlaneIdBuffer[p2]
                            || (mPlaneIdBuffer[p1] == mPlaneIdBuffer[p2] && mPositionBuffer[p1].y > mPositionBuffer[p2].y);
                    }),
                pointIndex);

            // Notify
            mGameEventHandler->OnPointCombustionBegin();
        }
    }

    //
    // Pick candidates for explosion
    //

    if (!mCombustionExplosionCandidates.empty())
    {
        size_t const maxExplosionPoints = std::min(
            size_t(15), // Magic number
            mCombustionExplosionCandidates.size());

        // Sort top N candidates by ignition temperature delta
        std::nth_element(
            mCombustionExplosionCandidates.data(),
            mCombustionExplosionCandidates.data() + maxExplosionPoints,
            mCombustionExplosionCandidates.data() + mCombustionExplosionCandidates.size(),
            [](auto const & t1, auto const & t2)
            {
                return std::get<1>(t1) > std::get<1>(t2);
            });

        // Calculate blast heat
        float const blastHeat =
            GameParameters::CombustionHeat
            * 1.5f // Arbitrary multiplier
            * GameParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>
            * gameParameters.CombustionHeatAdjustment
            * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

        // Explode these points
        for (size_t i = 0; i < maxExplosionPoints; ++i)
        {
            assert(i < mCombustionExplosionCandidates.size());

            auto const pointIndex = std::get<0>(mCombustionExplosionCandidates[i]);

            //
            // Explode!
            //

            float const blastRadius =
                mMaterialsBuffer[pointIndex].Structural->ExplosiveCombustionRadius
                * (gameParameters.IsUltraViolentMode ? 4.0f : 1.0f);

            float const blastForce =
                40000.0f // Magic number
                * mMaterialsBuffer[pointIndex].Structural->ExplosiveCombustionStrength;

            // Start explosion
            mShipPhysicsHandler->StartExplosion(
                currentSimulationTime,
                GetPlaneId(pointIndex),
                GetPosition(pointIndex),
                blastRadius,
                blastForce,
                blastHeat,
                ExplosionType::Combustion,
                gameParameters);

            // Notify explosion
            mGameEventHandler->OnCombustionExplosion(
                IsCachedUnderwater(pointIndex),
                1);

            // Transition state
            mCombustionStateBuffer[pointIndex].State = CombustionState::StateType::Exploded;
        }
    }

    //
    // Pick candidates for water reaction explosion
    //

    if (!mWaterReactionExplosionCandidates.empty())
    {
        size_t const maxExplosionPoints = std::min(
            size_t(25), // Magic number
            mWaterReactionExplosionCandidates.size());

        // Sort top N candidates by wetness
        std::nth_element(
            mWaterReactionExplosionCandidates.data(),
            mWaterReactionExplosionCandidates.data() + maxExplosionPoints,
            mWaterReactionExplosionCandidates.data() + mWaterReactionExplosionCandidates.size(),
            [](auto const & t1, auto const & t2)
            {
                return std::get<1>(t1) > std::get<1>(t2);
            });

        // Calculate explosion parameters

        float const blastHeat =
            GameParameters::WaterReactionHeat
            * GameParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>
            * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

        float const blastRadius =
            5.0f // Magic number
            * (gameParameters.IsUltraViolentMode ? 4.0f : 1.0f);

        float const blastForce = 3000000.0f; // Magic number

        // Explode these points
        for (size_t i = 0; i < maxExplosionPoints; ++i)
        {
            assert(i < mWaterReactionExplosionCandidates.size());

            auto const pointIndex = std::get<0>(mWaterReactionExplosionCandidates[i]);

            //
            // Explode!
            //

            mShipPhysicsHandler->StartExplosion(
                currentSimulationTime,
                GetPlaneId(pointIndex),
                GetPosition(pointIndex),
                blastRadius,
                blastForce,
                blastHeat,
                ExplosionType::Sodium,
                gameParameters);

            // Notify explosion
            mGameEventHandler->OnWaterReactionExplosion(
                IsCachedUnderwater(pointIndex),
                1);

            // Transition state
            mWaterReactionStateBuffer[pointIndex].State = WaterReactionState::StateType::Consumed;
        }
    }
}

void Points::UpdateCombustionHighFrequency(
    float /*currentSimulationTime*/,
    float dt,
    vec2f const & globalWindSpeed,
    std::optional<Wind::RadialWindField> const & radialWindField,
    GameParameters const & gameParameters)
{
    //
    // For all burning points, take care of following:
    // - Developing points: development up
    // - Burning points: heat generation
    // - Extinguishing points: development down
    //
    // And generally:
    // - Flame vector
    //

    // Heat generated by combustion in this step
    float const effectiveCombustionHeat =
        GameParameters::CombustionHeat
        * dt
        * gameParameters.CombustionHeatAdjustment;

    // Points that are not burning anymore after this step
    assert(mStoppedBurningPoints.empty());

    for (auto const pointIndex : mBurningPoints)
    {
        vec2f const & pointPosition = GetPosition(pointIndex);

        CombustionState & pointCombustionState = mCombustionStateBuffer[pointIndex];

        assert(pointCombustionState.State != CombustionState::StateType::NotBurning); // Otherwise it wouldn't be in this set

        //
        // Check if this point should stop developing/burning or start extinguishing faster
        //

        auto const currentState = pointCombustionState.State;

        if ((currentState == CombustionState::StateType::Developing_1
            || currentState == CombustionState::StateType::Developing_2
            || currentState == CombustionState::StateType::Burning
            || currentState == CombustionState::StateType::Extinguishing_Consumed)
            && (IsCachedUnderwater(pointIndex)
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

            // This point
            mTemperatureBuffer[pointIndex] =
                mMaterialIgnitionTemperatureBuffer[pointIndex]
                * gameParameters.IgnitionTemperatureAdjustment
                * 1.1f;

            // Neighbors
            for (auto const s : GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const otherEndpointIndex = s.OtherEndpointIndex;

                // Calculate direction coefficient so to prefer upwards direction:
                // 0.9 + 1.0*(1 - cos(theta)): 2.9 N, 0.9 S, 1.9 W and E
                vec2f const springDir = (GetPosition(otherEndpointIndex) - pointPosition).normalise();
                float const dirAlpha =
                    (0.9f + 1.0f * (1.0f - springDir.dot(GameParameters::GravityDir)));
                // No normalization: when using normalization flame does not propagate along rope

                // Add heat to the neighbor, diminishing with the neighbor's decay
                mTemperatureBuffer[otherEndpointIndex] +=
                    effectiveCombustionHeat
                    * dirAlpha
                    * mMaterialHeatCapacityReciprocalBuffer[otherEndpointIndex]
                    * mDecayBuffer[otherEndpointIndex];
            }
        }

        /* FUTUREWORK

        The following code emits smoke for burning particles, but there are rendering problems:
        Smoke should be drawn behind flames, hence GenericTexture's should be drawn in a layer that is earlier than flames.
        However, generic textures (smoke) have internal transparency, while flames have none; the Z test makes it so then
        that smoke at plane ID P shows the ship behind it, even though there are flames at plane IDs < P.
        The only way out that I may think of, at this moment, is to draw flames and generic textures alternatively, for each
        plane ID (!), or to make smoke fully opaque.

        //
        // Check if this point should emit smoke
        //

        if (pointCombustionState.State != CombustionState::StateType::NotBurning)
        {
            // See if we need to calculate the next emission timestamp
            if (pointCombustionState.NextSmokeEmissionSimulationTimestamp == 0.0f)
            {
                pointCombustionState.NextSmokeEmissionSimulationTimestamp =
                    currentSimulationTime
                    + GameRandomEngine::GetInstance().GenerateExponentialReal(
                        gameParameters.SmokeEmissionDensityAdjustment
                        * pointCombustionState.FlameDevelopment
                        / 1.5f); // CODEWORK: replace with Material's properties, precalc'd with gameParameters.SmokeEmissionDensityAdjustment
            }

            // See if it's time to emit smoke
            if (currentSimulationTime >= pointCombustionState.NextSmokeEmissionSimulationTimestamp)
            {
                //
                // Emit smoke
                //

                // Generate particle
                CreateEphemeralParticleHeavySmoke(
                    pointPosition,
                    GetTemperature(pointIndex),
                    currentSimulationTime,
                    GetPlaneId(pointIndex),
                    gameParameters);

                // Make sure we re-calculate the next emission timestamp
                pointCombustionState.NextSmokeEmissionSimulationTimestamp = 0.0f;
            }
        }
        */

        //
        // Run development/extinguishing state machine now
        //

        switch (pointCombustionState.State)
        {
            case CombustionState::StateType::Developing_1:
            {
                //
                // Develop up
                //
                // f(n-1) + 0.04*f(n-1): when starting from 0.1, after 61 steps (~1s) it's 1.1
                // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:0.1,%22valid%22:true}]&expression=f(n-1)%20+%200.105*f(n-1)&target=0&endTarget=25&range=true
                //

                pointCombustionState.FlameDevelopment +=
                    0.04f * pointCombustionState.FlameDevelopment;

                // Check whether it's time to transition to the next development phase
                if (pointCombustionState.FlameDevelopment > pointCombustionState.MaxFlameDevelopment + 0.1f)
                {
                    pointCombustionState.State = CombustionState::StateType::Developing_2;
                }

                break;
            }

            case CombustionState::StateType::Developing_2:
            {
                //
                // Develop down
                //

                // FlameDevelopment is now in the (MFD + epsilon, MFD) range
                auto const extraFlameDevelopment = pointCombustionState.FlameDevelopment - pointCombustionState.MaxFlameDevelopment;

                // Check whether it's time to transition to burning
                if (extraFlameDevelopment < 0.02f)
                {
                    pointCombustionState.State = CombustionState::StateType::Burning;
                    pointCombustionState.FlameDevelopment = pointCombustionState.MaxFlameDevelopment;
                }
                else
                {
                    // Keep converging to goal
                    pointCombustionState.FlameDevelopment -= 0.35f * extraFlameDevelopment;
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

                if (pointCombustionState.State == CombustionState::StateType::Extinguishing_Consumed)
                {
                    //
                    // f(n-1) - 0.0625*(1.01 - f(n-1)): when starting from 1, after 75 steps (1.5s) it's under 0.02
                    // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:1,%22valid%22:true}]&expression=f(n-1)%20-%200.0625*(1.01%20-%20f(n-1))&target=0&endTarget=80&range=true
                    //

                    pointCombustionState.FlameDevelopment -=
                        0.0625f
                        * (pointCombustionState.MaxFlameDevelopment - pointCombustionState.FlameDevelopment + 0.01f);
                }
                else if (pointCombustionState.State == CombustionState::StateType::Extinguishing_SmotheredRain)
                {
                    //
                    // f(n-1) - 0.075*f(n-1): when starting from 1, after 50 steps (1.0s) it's under 0.02
                    // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:1,%22valid%22:true}]&expression=f(n-1)%20-%200.075*f(n-1)&target=0&endTarget=75&range=true
                    //

                    pointCombustionState.FlameDevelopment -=
                        0.075f * pointCombustionState.FlameDevelopment;
                }
                else
                {
                    assert(pointCombustionState.State == CombustionState::StateType::Extinguishing_SmotheredWater);

                    //
                    // f(n-1) - 0.3*f(n-1): when starting from 1, after 10 steps (0.2s) it's under 0.02
                    // http://www.calcul.com/show/calculator/recursive?values=[{%22n%22:0,%22value%22:1,%22valid%22:true}]&expression=f(n-1)%20-%200.3*f(n-1)&target=0&endTarget=25&range=true
                    //

                    pointCombustionState.FlameDevelopment -=
                        0.3f * pointCombustionState.FlameDevelopment;
                }

                // Check whether we are done now
                if (pointCombustionState.FlameDevelopment <= 0.02f)
                {
                    //
                    // Stop burning
                    //

                    pointCombustionState.State = CombustionState::StateType::NotBurning;

                    // Remove point from set of burning points
                    mStoppedBurningPoints.emplace_back(pointIndex);
                }

                break;
            }

            case CombustionState::StateType::Burning:
            case CombustionState::StateType::Exploded:
            case CombustionState::StateType::NotBurning:
            {
                // Nothing to do here
                break;
            }
        }

        //
        // Calculate flame vector
        //
        // Note: the point might not be burning anymore, in case we've just extinguished it
        //

        Formulae::EvolveFlameGeometry(
            pointCombustionState.FlameVector,
            pointCombustionState.FlameWindRotationAngle,
            pointPosition,
            GetVelocity(pointIndex),
            globalWindSpeed,
            radialWindField);
    }

    //
    // Remove points that have stopped burning
    //

    if (!mStoppedBurningPoints.empty())
    {
        for (auto stoppedBurningPoint : mStoppedBurningPoints)
        {
            auto burningPointIt = std::find(
                mBurningPoints.cbegin(),
                mBurningPoints.cend(),
                stoppedBurningPoint);
            assert(burningPointIt != mBurningPoints.cend());
            mBurningPoints.erase(burningPointIt);
        }

        mStoppedBurningPoints.clear();
    }
}

void Points::ReorderBurningPointsForDepth()
{
    std::sort(
        mBurningPoints.begin(),
        mBurningPoints.end(),
        [this](auto p1, auto p2)
        {
            // Sort by plane and then by vertical position, so bottommost flames cover uppermost ones
            return mPlaneIdBuffer[p1] < mPlaneIdBuffer[p2]
                || (mPlaneIdBuffer[p1] == mPlaneIdBuffer[p2] && mPositionBuffer[p1].y > mPositionBuffer[p2].y);
        });
}

void Points::UpdateEphemeralParticles(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Transformation from desired velocity impulse to force
    float const randomWalkVelocityImpulseToForceCoefficient =
        GameParameters::AirMass
        / gameParameters.SimulationStepTimeDuration<float>;

    // Ocean surface displacement at bubbles surfacing
    float const oceanFloorDisplacementAtAirBubbleSurfacingSurfaceOffset =
        (gameParameters.DoDisplaceWater ? 1.0f : 0.0f)
        * 1.0f;

    // Visit all ephemeral particles
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
                        float const depth = GetCachedDepth(pointIndex);
                        if (depth <= 0.0f)
                        {
                            // Got to the surface, expire
                            ExpireEphemeralParticle(pointIndex);
                        }
                        else
                        {
                            //
                            // Update state
                            //

                            auto & state = mEphemeralParticleAttributes2Buffer[pointIndex].State.AirBubble;

                            // DeltaY

                            state.CurrentDeltaY = depth;

                            // Simulation lifetime

                            auto const simulationLifetime =
                                currentSimulationTime
                                - mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime;

                            state.SimulationLifetime = simulationLifetime;

                            //
                            // Update vortex
                            //

                            float const vortexValue =
                                state.VortexAmplitude
                                * PrecalcLoFreqSin.GetNearestPeriodic(
                                    state.NormalizedVortexAngularVelocity * simulationLifetime);

                            // Apply vortex to bubble
                            AddStaticForce(
                                pointIndex,
                                vec2f(
                                    vortexValue,
                                    0.0f));

                            //
                            // Displace ocean surface, if surfacing
                            //

                            if (depth < oceanFloorDisplacementAtAirBubbleSurfacingSurfaceOffset)
                            {
                                mParentWorld.DisplaceOceanSurfaceAt(
                                    GetPosition(pointIndex).x,
                                    (oceanFloorDisplacementAtAirBubbleSurfacingSurfaceOffset - depth) * 0.75f);  // Magic number

                                mGameEventHandler->OnAirBubbleSurfaced(1);
                            }
                        }
                    }

                    break;
                }

                case EphemeralType::Debris:
                {
                    // Check if expired
                    auto const elapsedSimulationLifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime;
                    auto const maxSimulationLifetime = mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime;
                    if (elapsedSimulationLifetime >= maxSimulationLifetime)
                    {
                        ExpireEphemeralParticle(pointIndex);

                        // Remember that ephemeral point elements are now dirty
                        mAreEphemeralPointElementsDirtyForRendering = true;
                    }
                    else
                    {
                        // Update alpha based off remaining time

                        float alpha = std::max(
                            1.0f - elapsedSimulationLifetime / maxSimulationLifetime,
                            0.0f);

                        mColorBuffer[pointIndex].w = alpha;
                        mIsEphemeralColorBufferDirty = true;
                    }

                    break;
                }

                case EphemeralType::Smoke:
                {
                    // Calculate progress
                    auto const elapsedSimulationLifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime;
                    assert(mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime > 0.0f);
                    float const lifetimeProgress =
                        elapsedSimulationLifetime
                        / mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime;

                    // Check if expired
                    if (lifetimeProgress >= 1.0f
                        || IsCachedUnderwater(pointIndex))
                    {
                        //
                        /// Expired
                        //

                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        //
                        // Still alive
                        //

                        // Update progress
                        mEphemeralParticleAttributes2Buffer[pointIndex].State.Smoke.LifetimeProgress = lifetimeProgress;
                        if (EphemeralState::SmokeState::GrowthType::Slow == mEphemeralParticleAttributes2Buffer[pointIndex].State.Smoke.Growth)
                        {
                            mEphemeralParticleAttributes2Buffer[pointIndex].State.Smoke.ScaleProgress =
                                std::min(1.0f, elapsedSimulationLifetime / 5.0f);
                        }
                        else
                        {
                            assert(EphemeralState::SmokeState::GrowthType::Fast == mEphemeralParticleAttributes2Buffer[pointIndex].State.Smoke.Growth);
                            mEphemeralParticleAttributes2Buffer[pointIndex].State.Smoke.ScaleProgress =
                                1.07f * (1.0f - exp(-3.0f * lifetimeProgress));
                        }

                        // Inject random walk in direction orthogonal to current velocity
                        float const randomWalkMagnitude =
                            0.3f * (static_cast<float>(GameRandomEngine::GetInstance().Choose<int>(2)) - 0.5f);
                        vec2f const deviationDirection =
                            GetVelocity(pointIndex).normalise().to_perpendicular();
                        AddStaticForce(
                            pointIndex,
                            deviationDirection * randomWalkMagnitude * randomWalkVelocityImpulseToForceCoefficient);
                    }

                    break;
                }

                case EphemeralType::Sparkle:
                {
                    // Check if expired
                    auto const elapsedSimulationLifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime;
                    auto const maxSimulationLifetime = mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime;
                    if (elapsedSimulationLifetime >= maxSimulationLifetime
                        || IsCachedUnderwater(pointIndex))
                    {
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        // Update progress based off remaining time
                        assert(maxSimulationLifetime > 0.0f);
                        mEphemeralParticleAttributes2Buffer[pointIndex].State.Sparkle.Progress =
                            elapsedSimulationLifetime / maxSimulationLifetime;
                    }

                    break;
                }

                case EphemeralType::WakeBubble:
                {
                    // Check if expired
                    auto const elapsedSimulationLifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[pointIndex].StartSimulationTime;
                    auto const maxSimulationLifetime = mEphemeralParticleAttributes2Buffer[pointIndex].MaxSimulationLifetime;
                    if (elapsedSimulationLifetime >= maxSimulationLifetime
                        || !IsCachedUnderwater(pointIndex))
                    {
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        // Update progress based off remaining time
                        assert(maxSimulationLifetime > 0.0f);
                        mEphemeralParticleAttributes2Buffer[pointIndex].State.WakeBubble.Progress =
                            elapsedSimulationLifetime / maxSimulationLifetime;
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

void Points::UpdateHighlights(GameWallClock::float_time currentWallClockTime)
{
    //
    // ElectricalElement highlights
    //

    auto constexpr ElectricalElementHighlightLifetime = 1s;

    for (auto it = mElectricalElementHighlightedPoints.begin(); it != mElectricalElementHighlightedPoints.end(); /* incremented in loop */)
    {
        // Calculate progress
        float const progress = GameWallClock::Progress(
            currentWallClockTime,
            it->StartTime,
            ElectricalElementHighlightLifetime);

        if (progress > 1.0f)
        {
            // Expire
            it = mElectricalElementHighlightedPoints.erase(it);
        }
        else
        {
            // Update
            it->Progress = progress;
            ++it;
        }
    }

    //
    // Circle
    //

    for (auto it = mCircleHighlightedPoints.begin(); it != mCircleHighlightedPoints.end(); /* incremented in loop */)
    {
        // Expected sequence when not renewed:
        // - Highlight created: SimulationStepsExperienced = 0
        // - Points::Update: SimulationStepsExperienced = 1
        // - Render
        // - Points::Update: SimulationStepsExperienced = 2 => removed
        // - Render (none)
        ++(it->SimulationStepsExperienced);
        if (it->SimulationStepsExperienced > 1)
        {
            // Expire
            it = mCircleHighlightedPoints.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Points::Query(ElementIndex pointElementIndex) const
{
    LogMessage("PointIndex: ", pointElementIndex, (nullptr != mMaterialsBuffer[pointElementIndex].Structural) ? (" (" + mMaterialsBuffer[pointElementIndex].Structural->Name) + ")" : "");
    LogMessage("P=", mPositionBuffer[pointElementIndex].toString(), " V=", mVelocityBuffer[pointElementIndex].toString());
    LogMessage("M=", mMassBuffer[pointElementIndex], " IP=", mInternalPressureBuffer[pointElementIndex], " W=", mWaterBuffer[pointElementIndex], " L=", mLightBuffer[pointElementIndex], " T=", mTemperatureBuffer[pointElementIndex], " Decay=", mDecayBuffer[pointElementIndex]);
    LogMessage("PlaneID: ", mPlaneIdBuffer[pointElementIndex], " ConnectedComponentID: ", mConnectedComponentIdBuffer[pointElementIndex]);
}

void Points::ColorPointForDebugging(
    ElementIndex pointIndex,
    rgbaColor const & color)
{
    mColorBuffer[pointIndex] = color.toVec4f();
    mIsWholeColorBufferDirty = true;
}

void Points::UploadAttributes(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    // Upload immutable attributes, if we haven't uploaded them yet
    if (mIsTextureCoordinatesBufferDirty)
    {
        shipRenderContext.UploadPointImmutableAttributes(mTextureCoordinatesBuffer.data());

        mIsTextureCoordinatesBufferDirty = false;
    }

    // Upload colors, if dirty
    if (mIsWholeColorBufferDirty)
    {
        renderContext.UploadShipPointColorsAsync(
            shipId,
            mColorBuffer.data(),
            0,
            mAllPointCount);

        mIsWholeColorBufferDirty = false;
        mIsEphemeralColorBufferDirty = false;
    }
    else if (mIsEphemeralColorBufferDirty)
    {
        // Only upload ephemeral particle portion
        renderContext.UploadShipPointColorsAsync(
            shipId,
            &(mColorBuffer.data()[mAlignedShipPointCount]),
            mAlignedShipPointCount,
            mEphemeralPointCount);

        mIsEphemeralColorBufferDirty = false;
    }

    //
    // Upload mutable attributes
    //

    shipRenderContext.UploadPointMutableAttributesStart();

    shipRenderContext.UploadPointMutableAttributes(
        mPositionBuffer.data(),
        mLightBuffer.data(),
        mWaterBuffer.data());

    if (mIsPlaneIdBufferNonEphemeralDirty)
    {
        if (mIsPlaneIdBufferEphemeralDirty)
        {
            // Whole

            shipRenderContext.UploadPointMutableAttributesPlaneId(
                mPlaneIdFloatBuffer.data(),
                0,
                mAllPointCount);

            mIsPlaneIdBufferEphemeralDirty = false;
        }
        else
        {
            // Just non-ephemeral portion

            shipRenderContext.UploadPointMutableAttributesPlaneId(
                mPlaneIdFloatBuffer.data(),
                0,
                mRawShipPointCount);
        }

        mIsPlaneIdBufferNonEphemeralDirty = false;
    }
    else if (mIsPlaneIdBufferEphemeralDirty)
    {
        // Just ephemeral portion

        shipRenderContext.UploadPointMutableAttributesPlaneId(
            &(mPlaneIdFloatBuffer.data()[mAlignedShipPointCount]),
            mAlignedShipPointCount,
            mEphemeralPointCount);

        mIsPlaneIdBufferEphemeralDirty = false;
    }

    // The following attributes never change for ephemeral particles,
    // hence after the first upload for reasonable defaults, we only
    // need to upload them for the ship's (structural == raw) points,
    // not for the ephemeral ones
    size_t const partialPointCount = mHaveWholeBuffersBeenUploadedOnce ? mRawShipPointCount : mAllPointCount;

    if (mIsDecayBufferDirty)
    {
        shipRenderContext.UploadPointMutableAttributesDecay(
            mDecayBuffer.data(),
            0,
            partialPointCount);

        mIsDecayBufferDirty = false;
    }

    if (renderContext.GetHeatRenderMode() != HeatRenderModeType::None)
    {
        renderContext.UploadShipPointTemperatureAsync(
            shipId,
            mTemperatureBuffer.data(),
            0,
            partialPointCount);
    }

    if (renderContext.GetStressRenderMode() != StressRenderModeType::None)
    {
        renderContext.UploadShipPointStressAsync(
            shipId,
            mStressBuffer.data(),
            0,
            partialPointCount);
    }

    if (renderContext.GetDebugShipRenderMode() == DebugShipRenderModeType::InternalPressure)
    {
        renderContext.UploadShipPointAuxiliaryDataAsync(
            shipId,
            mInternalPressureBuffer.data(),
            0,
            partialPointCount);
    }
    else if (renderContext.GetDebugShipRenderMode() == DebugShipRenderModeType::Strength)
    {
        renderContext.UploadShipPointAuxiliaryDataAsync(
            shipId,
            mStrengthBuffer.data(),
            0,
            partialPointCount);
    }

    shipRenderContext.UploadPointMutableAttributesEnd();

    mHaveWholeBuffersBeenUploadedOnce = true;
}

void Points::UploadNonEphemeralPointElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    bool const doUploadAllPoints = (DebugShipRenderModeType::Points == renderContext.GetDebugShipRenderMode());

    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    for (ElementIndex pointIndex : RawShipPoints())
    {
        if (doUploadAllPoints
            || mConnectedSpringsBuffer[pointIndex].ConnectedSprings.empty()) // orphaned
        {
            shipRenderContext.UploadElementPoint(pointIndex);
        }
    }
}

void Points::UploadFlames(Render::ShipRenderContext & shipRenderContext) const
{
    //
    // Flames are uploaded in this order:
    //  - Z order (guaranteed by internal sorting of mBurningPoints)
    //  - Background flames first (i.e. flames on ropes/springs) followed
    //    by foreground flames
    //
    // We use # of triangles as a heuristic for the point being on a chain,
    // and we use the *factory* ones to avoid sudden depth jumps when triangles are destroyed by fire
    //

    // Background
    for (auto const pointIndex : mBurningPoints)
    {
        if (mFactoryConnectedTrianglesBuffer[pointIndex].ConnectedTriangles.empty())
        {
            shipRenderContext.UploadShipBackgroundFlame(
                GetPlaneId(pointIndex),
                GetPosition(pointIndex),
                mCombustionStateBuffer[pointIndex].FlameVector,
                mCombustionStateBuffer[pointIndex].FlameWindRotationAngle,
                mCombustionStateBuffer[pointIndex].FlameDevelopment, // scale
                mRandomNormalizedUniformFloatBuffer[pointIndex]);
        }
    }

    // Foreground
    for (auto const pointIndex : mBurningPoints)
    {
        if (!mFactoryConnectedTrianglesBuffer[pointIndex].ConnectedTriangles.empty())
        {
            shipRenderContext.UploadShipForegroundFlame(
                GetPlaneId(pointIndex),
                GetPosition(pointIndex),
                mCombustionStateBuffer[pointIndex].FlameVector,
                mCombustionStateBuffer[pointIndex].FlameWindRotationAngle,
                mCombustionStateBuffer[pointIndex].FlameDevelopment, // scale
                mRandomNormalizedUniformFloatBuffer[pointIndex]);
        }
    }
}

void Points::UploadVectors(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    vec4f color;
    vec2f const * vectorBuffer = nullptr;
    float lengthAdjustment = 0.0f;

    switch (renderContext.GetVectorFieldRenderMode())
    {
        case VectorFieldRenderModeType::PointStaticForce:
        {
            color = vec4f(0.5f, 0.1f, 0.f, 1.0f);
            vectorBuffer = mStaticForceBuffer.data();
            lengthAdjustment = 0.00075f;

            break;
        }

        case VectorFieldRenderModeType::PointDynamicForce:
        {
            color = vec4f(1.0f, 0.266f, 0.16f, 1.0f);
            // First buffer implicitly
            assert(mDynamicForceBuffers.size() >= 1);
            vectorBuffer = mDynamicForceBuffers[0].data();
            lengthAdjustment = 0.000001f;

            break;
        }

        case VectorFieldRenderModeType::PointVelocity:
        {
            color = vec4f(0.203f, 0.552f, 0.219f, 1.0f);
            vectorBuffer = mVelocityBuffer.data();
            lengthAdjustment = 0.25f;

            break;
        }

        case VectorFieldRenderModeType::PointWaterMomentum:
        {
            color = vec4f(0.054f, 0.066f, 0.443f, 1.0f);
            vectorBuffer = mWaterMomentumBuffer.data();
            lengthAdjustment = 0.4f;

            break;
        }

        case VectorFieldRenderModeType::PointWaterVelocity:
        {
            color = vec4f(0.094f, 0.509f, 0.925f, 1.0f);
            vectorBuffer = mWaterVelocityBuffer.data();
            lengthAdjustment = 1.0f;

            break;
        }

        case VectorFieldRenderModeType::None:
        {
            return;
        }
    }

    shipRenderContext.UploadVectorsStart(mElementCount, color);

    for (auto const p : this->RawShipPoints())
    {
        shipRenderContext.UploadVector(
            GetPosition(p),
            mPlaneIdFloatBuffer[p],
            vectorBuffer[p],
            lengthAdjustment);
    }

    for (auto const p : this->EphemeralPoints())
    {
        if (GetEphemeralType(p) != EphemeralType::None)
        {
            shipRenderContext.UploadVector(
                GetPosition(p),
                mPlaneIdFloatBuffer[p],
                vectorBuffer[p],
                lengthAdjustment);
        }
    }

    shipRenderContext.UploadVectorsEnd();
}

void Points::UploadEphemeralParticles(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    //
    // Upload points and/or textures
    //

    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    if (mAreEphemeralPointElementsDirtyForRendering)
    {
        shipRenderContext.UploadElementEphemeralPointsStart();
    }

    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        switch (GetEphemeralType(pointIndex))
        {
            case EphemeralType::AirBubble:
            {
                auto const & state = mEphemeralParticleAttributes2Buffer[pointIndex].State.AirBubble;

                // Calculate scale based on lifetime
                float constexpr ScaleMax = 0.2f;
                float constexpr ScaleMin = 0.04f;
                float const scale =
                    ScaleMin + (ScaleMax - ScaleMin) * SmoothStep(0.0f, 2.0f, state.SimulationLifetime);

                shipRenderContext.UploadAirBubble(
                    GetPlaneId(pointIndex),
                    GetPosition(pointIndex),
                    scale,
                    std::min(0.6f, state.CurrentDeltaY)); // Alpha

                break;
            }

            case EphemeralType::Debris:
            {
                // Don't upload point unless there's been a change
                if (mAreEphemeralPointElementsDirtyForRendering)
                {
                    shipRenderContext.UploadElementEphemeralPoint(pointIndex);
                }

                break;
            }

            case EphemeralType::Smoke:
            {
                auto const & state = mEphemeralParticleAttributes2Buffer[pointIndex].State.Smoke;

                // Calculate scale
                float const scale = state.ScaleProgress;

                // Calculate alpha
                float const lifetimeProgress = state.LifetimeProgress;
                float const alpha =
                    SmoothStep(0.0f, 0.05f, lifetimeProgress)
                    - SmoothStep(0.7f, 1.0f, lifetimeProgress);

                // Upload smoke
                shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                    GetPlaneId(pointIndex),
                    state.PersonalitySeed,
                    state.TextureGroup,
                    GetPosition(pointIndex),
                    scale,
                    alpha);

                break;
            }

            case EphemeralType::Sparkle:
            {
                shipRenderContext.UploadSparkle(
                    GetPlaneId(pointIndex),
                    GetPosition(pointIndex),
                    GetVelocity(pointIndex),
                    mEphemeralParticleAttributes2Buffer[pointIndex].State.Sparkle.Progress);

                break;
            }

            case EphemeralType::WakeBubble:
            {
                auto const & state = mEphemeralParticleAttributes2Buffer[pointIndex].State.WakeBubble;

                shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                    GetPlaneId(pointIndex),
                    TextureFrameId(Render::GenericMipMappedTextureGroups::EngineWake, 0),
                    GetPosition(pointIndex),
                    0.10f + 1.22f * state.Progress, // Scale, magic formula
                    mRandomNormalizedUniformFloatBuffer[pointIndex] * 2.0f * Pi<float>, // Angle
                    1.0f - state.Progress); // Alpha

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

    if (mAreEphemeralPointElementsDirtyForRendering)
    {
        shipRenderContext.UploadElementEphemeralPointsEnd();

        // Not dirty anymore
        mAreEphemeralPointElementsDirtyForRendering = false;
    }
}

void Points::UploadHighlights(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    for (auto const & h : mElectricalElementHighlightedPoints)
    {
        shipRenderContext.UploadHighlight(
            HighlightModeType::ElectricalElement,
            GetPlaneId(h.PointIndex),
            GetPosition(h.PointIndex),
            5.0f, // HalfQuadSize, magic number
            h.HighlightColor,
            h.Progress);
    }

    for (auto const & h : mCircleHighlightedPoints)
    {
        shipRenderContext.UploadHighlight(
            HighlightModeType::Circle,
            GetPlaneId(h.PointIndex),
            GetPosition(h.PointIndex),
            4.0f, // HalfQuadSize, magic number
            h.HighlightColor,
            1.0f);
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
    //  - Current mass: augmented material mass + point's water mass, slowly converging to avoid discontinuities
    //  - Integration factor: integration factor time coefficient / current mass
    //

    float const densityAdjustedWaterMass = Formulae::CalculateWaterDensity(gameParameters.WaterTemperature, gameParameters);

    float const * restrict const augmentedMaterialMassBuffer = mAugmentedMaterialMassBuffer.data();
    float const * restrict const transientAdditionalMassBuffer = mTransientAdditionalMassBuffer.data();
    float const * restrict const waterBuffer = mWaterBuffer.data();
    float const * restrict const materialBuoyancyVolumeFillBuffer = mMaterialBuoyancyVolumeFillBuffer.data();
    float * restrict const massBuffer = mMassBuffer.data();
    float const * restrict const integrationFactorTimeCoefficientBuffer = mIntegrationFactorTimeCoefficientBuffer.data();
    float * restrict const integrationFactorBuffer = reinterpret_cast<float *>(mIntegrationFactorBuffer.data());

    size_t const count = GetBufferElementCount();
    for (size_t i = 0; i < count; ++i)
    {
        // The mass we want
        float const targetMass =
            augmentedMaterialMassBuffer[i]
            + transientAdditionalMassBuffer[i]
            + std::min(waterBuffer[i], materialBuoyancyVolumeFillBuffer[i]) * densityAdjustedWaterMass;

        // The mass we get: current mass slowly converging towards the mass we want
        // (nature abhors discontinuities)
        float const newMass = massBuffer[i] + (targetMass - massBuffer[i]) * 0.12f;

        assert(newMass > 0.0f);

        massBuffer[i] = newMass;

        integrationFactorBuffer[i * 2] = integrationFactorTimeCoefficientBuffer[i] / newMass;
        integrationFactorBuffer[i * 2 + 1] = integrationFactorTimeCoefficientBuffer[i] / newMass;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Points::CalculateCombustionDecayParameters(
    float combustionSpeedAdjustment,
    float dt)
{
    //
    // Combustion decay - we want it proportional to mass.
    //
    // We want to model the decay alpha as a quadratic law on the burning
    // particle's mass m: alpha = a * m^2 + b * m + c
    //
    // (Ideal) constraints:
    //
    //  * A cloth mass (0.6Kg) decays (reaches 0.5) in t=12 (simulation) seconds
    //  * An ~iron mass (800Kg) decays (reaches 0.5) in > 60 (simulation) seconds => alpha = 0.980194129
    //  * The largest mass (2400Kg) decays (reaches 0.5) in a ton of (simulation) seconds => alpha = 0.9998
    //
    // The constraints above are expressed with the following formulas:
    //  n = t / (gameParameters.CombustionSpeedAdjustment * dt)
    //  alpha_i ^ n_i = 0.5
    //

    assert(mMaterialDatabase.GetLargestStructuralMass() == 2400.0f); // Sentinel to recalc below in case mass changes

    float constexpr m1 = 0.6f;
    float constexpr t1 = 12.0f;
    float const n1 = t1 / (combustionSpeedAdjustment * dt);
    float const alpha1 = std::pow(0.5f, 1.0f / n1); // 0.956739426

    float constexpr m2 = 800.0f;
    float constexpr t2 = 26.5284f;
    float const n2 = t2 / (combustionSpeedAdjustment * dt);
    float const alpha2 = std::pow(0.5f, 1.0f / n2); // 0.980194151

    float constexpr m3 = 2400.0f;
    float constexpr t3 = 2653.19f;
    float const n3 = t3 / (combustionSpeedAdjustment * dt);
    float const alpha3 = std::pow(0.5f, 1.0f / n3); // 0.9998

    // den = (x1−x2)(x1−x3)(x2−x3)
    // a = x3(y2−y1)+x2(y1−y3)+x1(y3−y2) / den
    // b = x^21(y2−y3)+x^23(y1−y2)+x^22(y3−y1) / den
    // c = x^22(x3y1−x1y3)+x2(x^21y3−x^23y1)+x1x3(x3−x1)y2
    float const den = (m1 - m2) * (m1 - m3) * (m2 - m3);
    float const a_num = m3 * (alpha2 - alpha1) + m2 * (alpha1 - alpha3) + m1 * (alpha3 - alpha2);
    float const b_num = m1 * m1 * (alpha2 - alpha3) + m3 * m3 * (alpha1 - alpha2) + m2 * m2 * (alpha3 - alpha1);
    float const c_num = m2 * m2 * (m3 * alpha1 - m1 * alpha3) + m2 * (m1 * m1 * alpha3 - m3 * m3 * alpha1) + m1 * m3 * (m3 - m1) * alpha2;

    mCombustionDecayAlphaFunctionA = a_num / den;
    mCombustionDecayAlphaFunctionB = b_num / den;
    mCombustionDecayAlphaFunctionC = c_num / den;
}

ElementIndex Points::FindFreeEphemeralParticle(
    float currentSimulationTime,
    bool doForce)
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
        auto const lifetime = currentSimulationTime - mEphemeralParticleAttributes1Buffer[p].StartSimulationTime;
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

    if (!doForce)
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