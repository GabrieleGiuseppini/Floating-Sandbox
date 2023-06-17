/***************************************************************************************
 * Original Author:      Gabriele Giuseppini
 * Created:              2018-05-12
 * Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include <cmath>

namespace Physics {

void Springs::Add(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    Octant factoryPointAOctant,
    Octant factoryPointBOctant,
    SuperTrianglesVector const & superTriangles,
    ElementCount coveringTrianglesCount,
    Points const & points)
{
    ElementIndex const springIndex = static_cast<ElementIndex>(mIsDeletedBuffer.GetCurrentPopulatedSize());

    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex);

    mFactoryEndpointOctantsBuffer.emplace_back(factoryPointAOctant, factoryPointBOctant);

    mSuperTrianglesBuffer.emplace_back(superTriangles);
    mFactorySuperTrianglesBuffer.emplace_back(superTriangles);

    assert(coveringTrianglesCount >= superTriangles.size()); // Covering triangles count includes super triangles
    mCoveringTrianglesCountBuffer.emplace_back(coveringTrianglesCount);

    // Strain threshold is average, and randomized - +/-
    float constexpr RandomWidth = 0.7f; // 70%: 35% less or 35% more
    float const averageStrainThreshold = (points.GetStructuralMaterial(pointAIndex).StrainThresholdFraction + points.GetStructuralMaterial(pointBIndex).StrainThresholdFraction) / 2.0f;
    float const strainThreshold = averageStrainThreshold
        * (1.0f - RandomWidth / 2.0f + RandomWidth * points.GetRandomNormalizedUniformPersonalitySeed(pointAIndex));

    mStrainStateBuffer.emplace_back(
        0.0f, // Breaking elongation recalculated later
        strainThreshold,
        false);

    mFactoryRestLengthBuffer.emplace_back((points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length());
    mRestLengthBuffer.emplace_back((points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length());

    // Dynamics coefficients recalculated later, but stiffness grows slowly and shrinks fast, hence we want to start high
    mStiffnessCoefficientBuffer.emplace_back(std::numeric_limits<float>::max());
    mDampingCoefficientBuffer.emplace_back(0.0f);

    // Stiffness is average
    float const averageStiffness =
        (points.GetStructuralMaterial(pointAIndex).Stiffness + points.GetStructuralMaterial(pointBIndex).Stiffness)
        / 2.0f;

    // Strength is average
    float const averageStrength =
        (points.GetStrength(pointAIndex) + points.GetStrength(pointBIndex))
        / 2.0f;

    // Melting temperature is average
    float const averageMeltingTemperature =
        (points.GetStructuralMaterial(pointAIndex).MeltingTemperature + points.GetStructuralMaterial(pointBIndex).MeltingTemperature)
        / 2.0f;

    mMaterialPropertiesBuffer.emplace_back(
        averageStiffness,
        averageStrength,
        averageMeltingTemperature,
        CalculateExtraMeltingInducedTolerance(averageStrength));

    // Base structural material is arbitrarily the weakest of the two;
    // only affects sound and name, anyway
    mBaseStructuralMaterialBuffer.emplace_back(
        points.GetStructuralMaterial(pointAIndex).Strength < points.GetStructuralMaterial(pointBIndex).Strength
        ? &(points.GetStructuralMaterial(pointAIndex))
        : &(points.GetStructuralMaterial(pointBIndex)));

    // If both nodes are rope, then the spring is rope
    // (non-rope <-> rope springs are "connections" and not to be treated as ropes)
    mIsRopeBuffer.emplace_back(points.IsRope(pointAIndex) && points.IsRope(pointBIndex));

    // Spring is permeable by default - will be changed later
    mWaterPermeabilityBuffer.emplace_back(1.0f);

    // Heat properties are average
    float const thermalConductivity =
        (points.GetStructuralMaterial(pointAIndex).ThermalConductivity + points.GetStructuralMaterial(pointBIndex).ThermalConductivity)
        / 2.0f;
    mMaterialThermalConductivityBuffer.emplace_back(thermalConductivity);

    // Calculate parameters for this spring
    UpdateCoefficients(
        springIndex,
        mCurrentNumMechanicalDynamicsIterations,
        mCurrentSpringStiffnessAdjustment,
        mCurrentSpringDampingAdjustment,
        mCurrentSpringStrengthAdjustment,
        CalculateSpringStrengthIterationsAdjustment(mCurrentNumMechanicalDynamicsIterationsAdjustment),
        mCurrentMeltingTemperatureAdjustment,
        points);
}

void Springs::Destroy(
    ElementIndex springElementIndex,
    DestroyOptions destroyOptions,
    GameParameters const & gameParameters,
    Points const & points)
{
    assert(!IsDeleted(springElementIndex));

    // Invoke destroy handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandleSpringDestroy(
        springElementIndex,
        !!(destroyOptions & Springs::DestroyOptions::DestroyAllTriangles),
        gameParameters);

    // Fire spring break event, unless told otherwise
    if (!!(destroyOptions & Springs::DestroyOptions::FireBreakEvent))
    {
        mGameEventHandler->OnBreak(
            GetBaseStructuralMaterial(springElementIndex),
            mParentWorld.GetOceanSurface().IsUnderwater(GetEndpointAPosition(springElementIndex, points)), // Arbitrary
            1);
    }

    // Zero out our dynamics coefficients, so that we can still calculate Hooke's
    // and damping forces for this spring without running the risk of
    // affecting non-deleted points
    mStiffnessCoefficientBuffer[springElementIndex] = 0.0f;
    mDampingCoefficientBuffer[springElementIndex] = 0.0f;

    // Flag ourselves as deleted
    mIsDeletedBuffer[springElementIndex] = true;
}

void Springs::Restore(
    ElementIndex springElementIndex,
    GameParameters const & gameParameters,
    Points const & points)
{
    assert(IsDeleted(springElementIndex));

    // Clear the deleted flag
    mIsDeletedBuffer[springElementIndex] = false;

    // Recalculate coefficients for this spring
    UpdateCoefficients(
        springElementIndex,
        mCurrentNumMechanicalDynamicsIterations,
        mCurrentSpringStiffnessAdjustment,
        mCurrentSpringDampingAdjustment,
        mCurrentSpringStrengthAdjustment,
        CalculateSpringStrengthIterationsAdjustment(mCurrentNumMechanicalDynamicsIterationsAdjustment),
        mCurrentMeltingTemperatureAdjustment,
        points);

    // Invoke restore handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandleSpringRestore(
        springElementIndex,
        gameParameters);
}

void Springs::UpdateForGameParameters(
    GameParameters const & gameParameters,
    Points const & points)
{
    if (gameParameters.NumMechanicalDynamicsIterations<float>() != mCurrentNumMechanicalDynamicsIterations
        || gameParameters.NumMechanicalDynamicsIterationsAdjustment != mCurrentNumMechanicalDynamicsIterationsAdjustment
        || gameParameters.SpringStiffnessAdjustment != mCurrentSpringStiffnessAdjustment
        || gameParameters.SpringDampingAdjustment != mCurrentSpringDampingAdjustment
        || gameParameters.SpringStrengthAdjustment != mCurrentSpringStrengthAdjustment
        || gameParameters.MeltingTemperatureAdjustment != mCurrentMeltingTemperatureAdjustment)
    {
        // Update our version of the parameters
        mCurrentNumMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<float>();
        mCurrentNumMechanicalDynamicsIterationsAdjustment = gameParameters.NumMechanicalDynamicsIterationsAdjustment;
        mCurrentSpringStiffnessAdjustment = gameParameters.SpringStiffnessAdjustment;
        mCurrentSpringDampingAdjustment = gameParameters.SpringDampingAdjustment;
        mCurrentSpringStrengthAdjustment = gameParameters.SpringStrengthAdjustment;
        mCurrentMeltingTemperatureAdjustment = gameParameters.MeltingTemperatureAdjustment;

        // Recalc whole
        UpdateCoefficientsForPartition(
            0, 1,
            mCurrentNumMechanicalDynamicsIterations,
            mCurrentSpringStiffnessAdjustment,
            mCurrentSpringDampingAdjustment,
            mCurrentSpringStrengthAdjustment,
            CalculateSpringStrengthIterationsAdjustment(mCurrentNumMechanicalDynamicsIterationsAdjustment),
            mCurrentMeltingTemperatureAdjustment,
            points);
    }
}

void Springs::UploadElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    // Either upload all springs, or just the edge springs
    bool const doUploadAllSprings =
        DebugShipRenderModeType::Springs == renderContext.GetDebugShipRenderMode();

    // Ropes are uploaded as springs only if DebugRenderMode is springs or edge springs
    bool const doUploadRopesAsSprings =
        DebugShipRenderModeType::Springs == renderContext.GetDebugShipRenderMode()
        || DebugShipRenderModeType::EdgeSprings == renderContext.GetDebugShipRenderMode();

    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    for (ElementIndex i : *this)
    {
        // Only upload non-deleted springs that are not covered by two super-triangles, unless
        // we are in springs render mode
        if (!mIsDeletedBuffer[i])
        {
            if (IsRope(i) && !doUploadRopesAsSprings)
            {
                shipRenderContext.UploadElementRope(
                    GetEndpointAIndex(i),
                    GetEndpointBIndex(i));
            }
            else if (
                mCoveringTrianglesCountBuffer[i] < 2
                || doUploadAllSprings
                || IsRope(i))
            {
                shipRenderContext.UploadElementSpring(
                    GetEndpointAIndex(i),
                    GetEndpointBIndex(i));
            }
        }
    }
}

void Springs::UploadStressedSpringElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            if (mStrainStateBuffer[i].IsStressed)
            {
                shipRenderContext.UploadElementStressedSpring(
                    GetEndpointAIndex(i),
                    GetEndpointBIndex(i));
            }
        }
    }
}

void Springs::UpdateForStrains(
    GameParameters const & gameParameters,
    Points & points,
    StressRenderModeType stressRenderMode)
{
    if (stressRenderMode == StressRenderModeType::None)
    {
        InternalUpdateForStrains<false>(gameParameters, points);
    }
    else
    {
        InternalUpdateForStrains<true>(gameParameters, points);
    }
}

////////////////////////////////////////////////////////////////////

template<bool DoUpdateStress>
void Springs::InternalUpdateForStrains(
    GameParameters const & gameParameters,
    Points & points)
{
    float constexpr StrainLowWatermark = 0.08f; // Less than this multiplier to become non-stressed

    OceanSurface const & oceanSurface = mParentWorld.GetOceanSurface();

    // Visit all springs
    for (ElementIndex s : *this)
    {
        // Avoid breaking deleted springs
        if (!mIsDeletedBuffer[s])
        {
            auto & strainState = mStrainStateBuffer[s];

            // Calculate strain
            float const strain = GetLength(s, points) - mRestLengthBuffer[s];
            float const absStrain = std::abs(strain);

            // Check against breaking elongation
            float const breakingElongation = strainState.BreakingElongation;
            if (absStrain > breakingElongation)
            {
                // It's broken!

                // Destroy this spring
                this->Destroy(
                    s,
                    DestroyOptions::FireBreakEvent // Notify Break
                    | DestroyOptions::DestroyAllTriangles,
                    gameParameters,
                    points);
            }
            else
            {
                if (strainState.IsStressed)
                {
                    // Stressed spring...
                    // ...see if should un-stress it

                    if (absStrain < StrainLowWatermark * breakingElongation)
                    {
                        // It's not stressed anymore
                        strainState.IsStressed = false;
                    }
                }
                else
                {
                    // Not stressed spring
                    // ...see if should stress it

                    if (absStrain > strainState.StrainThresholdFraction * breakingElongation)
                    {
                        // It's stressed!
                        strainState.IsStressed = true;

                        // Notify stress
                        mGameEventHandler->OnStress(
                            GetBaseStructuralMaterial(s),
                            oceanSurface.IsUnderwater(GetEndpointAPosition(s, points)), // Arbitrary
                            1);
                    }
                }

                // Update stress
                if constexpr (DoUpdateStress)
                {
                    float const stress = strain / breakingElongation; // Between -1.0 and +1.0
                    
                    if (std::abs(stress) > std::abs(points.GetStress(GetEndpointAIndex(s))))
                    {
                        points.SetStress(
                            GetEndpointAIndex(s),
                            stress);
                    }

                    if (std::abs(stress) > std::abs(points.GetStress(GetEndpointBIndex(s))))
                    {
                        points.SetStress(
                            GetEndpointBIndex(s),
                            stress);
                    }
                }
            }
        }
    }
}

void Springs::UpdateCoefficientsForPartition(
    ElementIndex partition,
    ElementIndex partitionCount,
    float numMechanicalDynamicsIterations,
    float stiffnessAdjustment,
    float dampingAdjustment,
    float strengthAdjustment,
    float strengthIterationsAdjustment,
    float meltingTemperatureAdjustment,
    Points const & points)
{
    // Recalc all parameters
    ElementCount const partitionSize = (GetElementCount() / partitionCount) + ((GetElementCount() % partitionCount) ? 1 : 0);
    ElementCount const startSpringIndex = partition * partitionSize;
    ElementCount const endSpringIndex = std::min(startSpringIndex + partitionSize, GetElementCount());
    for (ElementIndex s = startSpringIndex; s < endSpringIndex; ++s)
    {
        if (!IsDeleted(s))
        {
            inline_UpdateCoefficients(
                s,
                numMechanicalDynamicsIterations,
                stiffnessAdjustment,
                dampingAdjustment,
                strengthAdjustment,
                strengthIterationsAdjustment,
                meltingTemperatureAdjustment,
                points);
        }
    }
}

void Springs::UpdateCoefficients(
    ElementIndex springIndex,
    float numMechanicalDynamicsIterations,
    float stiffnessAdjustment,
    float dampingAdjustment,
    float strengthAdjustment,
    float strengthIterationsAdjustment,
    float meltingTemperatureAdjustment,
    Points const & points)
{
    inline_UpdateCoefficients(
        springIndex,
        numMechanicalDynamicsIterations,
        stiffnessAdjustment,
        dampingAdjustment,
        strengthAdjustment,
        strengthIterationsAdjustment,
        meltingTemperatureAdjustment,
        points);
}

void Springs::inline_UpdateCoefficients(
    ElementIndex springIndex,
    float numMechanicalDynamicsIterations,
    float stiffnessAdjustment,
    float dampingAdjustment,
    float strengthAdjustment,
    float strengthIterationsAdjustment,
    float meltingTemperatureAdjustment,
    Points const & points)
{
    auto const endpointAIndex = GetEndpointAIndex(springIndex);
    auto const endpointBIndex = GetEndpointBIndex(springIndex);

    float const massFactor =
        (points.GetAugmentedMaterialMass(endpointAIndex) * points.GetAugmentedMaterialMass(endpointBIndex))
        / (points.GetAugmentedMaterialMass(endpointAIndex) + points.GetAugmentedMaterialMass(endpointBIndex));

    float const dt = GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations;

    // Note: in 1.14 the spring temperature was the average of the two points.
    // Differences in temperature between adjacent points made it so that springs'
    // melting was widely underestimated.
    // In reality, a spring is as "soft" as its softest point.
    float const springTemperature = std::max(
        points.GetTemperature(endpointAIndex),
        points.GetTemperature(endpointBIndex));

    // Excedence of temperature over melting temperature; might be negative
    // if we're below the melting temperature
    float const meltingOverheat =
        springTemperature
        - GetMaterialMeltingTemperature(springIndex) * meltingTemperatureAdjustment;

    //
    // Stiffness coefficient
    //
    // The "stiffness coefficient" is the factor which, once multiplied with the spring displacement,
    // yields the spring force, according to Hooke's law.
    //
    // We calculate the coefficient so that the two forces applied to each of the two masses produce a resulting
    // change in position equal to a fraction SpringReductionFraction * adjustment of the spring displacement,
    // in the time interval of a single mechanical dynamics simulation.
    //
    // After one iteration the spring displacement dL = L - L0 is reduced to:
    //  dL * (1-SRF)
    // where SRF is the (adjusted) SpringReductionFraction parameter. After N iterations this would be:
    //  dL * (1-SRF)^N
    //
    // The reduction adjustment is both the material-specific adjustment and the global game adjustment.
    //
    // If the endpoints are melting, their temperature also controls the stiffness - the higher the temperature,
    // above the melting point, the lower the stiffness; this is achieved with a smoothed multiplier with the following
    // edges:
    //  T <= Tm                    :    1.0
    //  T >= Tm + DeltaMeltingTMax :    ~< 1.0 (== MinStiffnessFraction, asymptote)
    //

    // Asymptote
    // NOTE: This value should be adjusted based on the number of spring iterations we perform
    // per simulation step
    float constexpr MinStiffnessFraction = 0.0002f;

    // We reach max softness at T+200
    float const meltDepthFraction = SmoothStep(0.0f, 200.0f, meltingOverheat);

    // 1.0 when not melting, MinStiffnessFraction when melting "a lot"
    float const meltMultiplier = Mix(1.0f, MinStiffnessFraction, meltDepthFraction);

    // Our desired stiffness coefficient
    float const desiredStiffnessCoefficient =
        GameParameters::SpringReductionFraction
        * GetMaterialStiffness(springIndex)
        * stiffnessAdjustment
        * massFactor
        / (dt * dt)
        * meltMultiplier;

    // If the coefficient is growing (spring is becoming more stiff), then
    // approach the desired stiffness coefficient slowly,
    // or else we have too much discontinuity and might explode
    if (desiredStiffnessCoefficient > mStiffnessCoefficientBuffer[springIndex])
    {
        mStiffnessCoefficientBuffer[springIndex] +=
            0.03f // 0.03: ~76 steps to 1/10th off target
            * (desiredStiffnessCoefficient - mStiffnessCoefficientBuffer[springIndex]);
    }
    else
    {
        // Sudden decrease
        mStiffnessCoefficientBuffer[springIndex] = desiredStiffnessCoefficient;
    }

    //
    // Damping coefficient
    //
    // Magnitude of the drag force on the relative velocity component along the spring.
    //

    mDampingCoefficientBuffer[springIndex] =
        GameParameters::SpringDampingCoefficient
        * dampingAdjustment
        * massFactor
        / dt;

    //
    // Breaking elongation
    //
    // The breaking elongation - i.e. the max delta L, aka displacement tolerance - depends on:
    //  - The material strength and the strength adjustment
    //  - The spring's decay (which itself is a function of the endpoints' decay)
    //  - If the endpoints are melting, their temperature - so to keep springs intact while melting makes them longer
    //  - The actual number of mechanics iterations we'll be performing
    //
    // The breaking elongation is the strength multiplied with the spring's rest length, so that it's ready to be
    // compared against the spring's absolute delta L without having to divide the delta L by the rest length
    //

    // Decay of spring == avg of two endpoints' decay
    float const springDecay =
        (points.GetDecay(endpointAIndex) + points.GetDecay(endpointBIndex))
        / 2.0f;

    // If we're melting, the current spring length, when longer than the
    // previous rest length, is also its new rest length - but no more than a few times
    // the factory rest length, or else springs become abnormally-long spikes.
    // When cooling again, we leave the rest length at its maximum - modeling permanent deformation.
    if (meltingOverheat > 0.0f)
    {
        SetRestLength(
            springIndex,
            Clamp(
                GetLength(springIndex, points),
                GetRestLength(springIndex),
                mFactoryRestLengthBuffer[springIndex] * 2.0f));
    }

    mStrainStateBuffer[springIndex].BreakingElongation =
        GetMaterialStrength(springIndex)
        * strengthAdjustment
        * 0.839501f // Magic number: from 1.14, after #iterations increased from 24 to 30
        * 0.643389f // Magic number: in 1.15.2 we've shortened the simulation time step from 0.2 to 0.156
        * strengthIterationsAdjustment
        * springDecay
        * GetRestLength(springIndex) // To make strain comparison independent from rest length
        * (1.0f + GetExtraMeltingInducedTolerance(springIndex) * meltDepthFraction); // When melting, springs are more tolerant to elongation
}

}