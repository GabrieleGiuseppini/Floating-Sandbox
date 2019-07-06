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
    int32_t factoryPointAOctant,
    int32_t factoryPointBOctant,
    SuperTrianglesVector const & superTriangles,
    Characteristics characteristics,
    Points const & points)
{
    ElementIndex const springIndex = static_cast<ElementIndex>(mIsDeletedBuffer.GetCurrentPopulatedSize());

    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex);

    mFactoryEndpointOctantsBuffer.emplace_back(factoryPointAOctant, factoryPointBOctant);

    mSuperTrianglesBuffer.emplace_back(superTriangles);
    mFactorySuperTrianglesBuffer.emplace_back(superTriangles);

    // Strength is average
    float const averageStrength =
        (points.GetStructuralMaterial(pointAIndex).Strength + points.GetStructuralMaterial(pointBIndex).Strength)
        / 2.0f;
    mMaterialStrengthBuffer.emplace_back(averageStrength);

    // Breaking length recalculated later
    mBreakingLengthBuffer.emplace_back(0.0f);

    // Stiffness is average
    float const stiffness =
        (points.GetStructuralMaterial(pointAIndex).Stiffness + points.GetStructuralMaterial(pointBIndex).Stiffness)
        / 2.0f;
    mMaterialStiffnessBuffer.emplace_back(stiffness);

    mRestLengthBuffer.emplace_back((points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length());

    // Coefficients recalculated later
    mCoefficientsBuffer.emplace_back(0.0f, 0.0f);

    mMaterialCharacteristicsBuffer.emplace_back(characteristics);

    // Base structural material is arbitrarily the weakest of the two;
    // only affects sound and name, anyway
    mBaseStructuralMaterialBuffer.emplace_back(
        points.GetStructuralMaterial(pointAIndex).Strength < points.GetStructuralMaterial(pointBIndex).Strength
        ? &points.GetStructuralMaterial(pointAIndex)
        : &points.GetStructuralMaterial(pointBIndex));

    // Spring is impermeable if it's a hull spring (i.e. if at least one endpoint is hull)
    mMaterialWaterPermeabilityBuffer.emplace_back(
        Characteristics::None != (characteristics & Characteristics::Hull)
        ? 0.0f
        : 1.0f);

    // Heat properties are average
    float const thermalConductivity =
        (points.GetStructuralMaterial(pointAIndex).ThermalConductivity + points.GetStructuralMaterial(pointBIndex).ThermalConductivity)
        / 2.0f;
    mMaterialThermalConductivityBuffer.emplace_back(thermalConductivity);
    float const meltingTemperature =
        (points.GetStructuralMaterial(pointAIndex).MeltingTemperature + points.GetStructuralMaterial(pointBIndex).MeltingTemperature)
        / 2.0f;
    mMaterialMeltingTemperatureBuffer.emplace_back(meltingTemperature);

    mIsStressedBuffer.emplace_back(false);

    mIsBombAttachedBuffer.emplace_back(false);

    // Calculate parameters for this spring
    UpdateForDecayAndTemperatureAndGameParameters(
        springIndex,
        mCurrentNumMechanicalDynamicsIterations,
        mCurrentSpringStiffnessAdjustment,
        mCurrentSpringDampingAdjustment,
        mCurrentSpringStrengthAdjustment,
        CalculateSpringStrengthIterationsAdjustment(mCurrentNumMechanicalDynamicsIterationsAdjustment),
        points);
}

void Springs::Destroy(
    ElementIndex springElementIndex,
    DestroyOptions destroyOptions,
    GameParameters const & gameParameters,
    Points const & points)
{
    assert(springElementIndex < mElementCount);
    assert(!IsDeleted(springElementIndex));

    // Invoke destroy handler
    if (!!mDestroyHandler)
    {
        mDestroyHandler(
            springElementIndex,
            !!(destroyOptions & Springs::DestroyOptions::DestroyAllTriangles),
            gameParameters);
    }

    // Fire spring break event, unless told otherwise
    if (!!(destroyOptions & Springs::DestroyOptions::FireBreakEvent))
    {
        mGameEventHandler->OnBreak(
            GetBaseStructuralMaterial(springElementIndex),
            mParentWorld.IsUnderwater(GetEndpointAPosition(springElementIndex, points)), // Arbitrary
            1);
    }

    // Zero out our coefficients, so that we can still calculate Hooke's
    // and damping forces for this spring without running the risk of
    // affecting non-deleted points
    mCoefficientsBuffer[springElementIndex].StiffnessCoefficient = 0.0f;
    mCoefficientsBuffer[springElementIndex].DampingCoefficient = 0.0f;

    // Flag ourselves as deleted
    mIsDeletedBuffer[springElementIndex] = true;
}

void Springs::Restore(
    ElementIndex springElementIndex,
    GameParameters const & gameParameters,
    Points const & points)
{
    assert(springElementIndex < mElementCount);
    assert(IsDeleted(springElementIndex));

    // Clear the delete flag
    mIsDeletedBuffer[springElementIndex] = false;

    // Recalculate parameters for this spring
    UpdateForDecayAndTemperatureAndGameParameters(
        springElementIndex,
        mCurrentNumMechanicalDynamicsIterations,
        mCurrentSpringStiffnessAdjustment,
        mCurrentSpringDampingAdjustment,
        mCurrentSpringStrengthAdjustment,
        CalculateSpringStrengthIterationsAdjustment(mCurrentNumMechanicalDynamicsIterationsAdjustment),
        points);

    // Invoke restore handler
    if (!!mRestoreHandler)
    {
        mRestoreHandler(
            springElementIndex,
            gameParameters);
    }
}

void Springs::UpdateForGameParameters(
    GameParameters const & gameParameters,
    Points const & points)
{
    if (gameParameters.NumMechanicalDynamicsIterations<float>() != mCurrentNumMechanicalDynamicsIterations
        || gameParameters.NumMechanicalDynamicsIterationsAdjustment != mCurrentNumMechanicalDynamicsIterationsAdjustment
        || gameParameters.SpringStiffnessAdjustment != mCurrentSpringStiffnessAdjustment
        || gameParameters.SpringDampingAdjustment != mCurrentSpringDampingAdjustment
        || gameParameters.SpringStrengthAdjustment != mCurrentSpringStrengthAdjustment)
    {
        // Recalc
        UpdateForDecayAndTemperatureAndGameParameters(
            gameParameters,
            points);

        assert(mCurrentNumMechanicalDynamicsIterations == gameParameters.NumMechanicalDynamicsIterations<float>());
        assert(mCurrentNumMechanicalDynamicsIterationsAdjustment == gameParameters.NumMechanicalDynamicsIterationsAdjustment);
        assert(mCurrentSpringStiffnessAdjustment == gameParameters.SpringStiffnessAdjustment);
        assert(mCurrentSpringDampingAdjustment == gameParameters.SpringDampingAdjustment);
        assert(mCurrentSpringStrengthAdjustment == gameParameters.SpringStrengthAdjustment);
    }
}

void Springs::UpdateForDecayAndTemperatureAndGameParameters(
    GameParameters const & gameParameters,
    Points const & points)
{
    //
    // Assumption: decay and temperature have changed; parameters might have (but most likely not)
    //

    // Recalc
    UpdateForDecayAndTemperatureAndGameParameters(
        gameParameters.NumMechanicalDynamicsIterations<float>(),
        gameParameters.NumMechanicalDynamicsIterationsAdjustment,
        gameParameters.SpringStiffnessAdjustment,
        gameParameters.SpringDampingAdjustment,
        gameParameters.SpringStrengthAdjustment,
        points);

    // Remember the new values
    mCurrentNumMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<float>();
    mCurrentNumMechanicalDynamicsIterationsAdjustment = gameParameters.NumMechanicalDynamicsIterationsAdjustment;
    mCurrentSpringStiffnessAdjustment = gameParameters.SpringStiffnessAdjustment;
    mCurrentSpringDampingAdjustment = gameParameters.SpringDampingAdjustment;
    mCurrentSpringStrengthAdjustment = gameParameters.SpringStrengthAdjustment;
}

void Springs::UploadElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    // Either upload all springs, or just the edge springs
    bool const doUploadAllSprings = (DebugShipRenderMode::Springs == renderContext.GetDebugShipRenderMode());

    // Ropes are uploaded as springs only if DebugRenderMode is springs or edge springs
    bool const doUploadRopesAsSprings = (
        DebugShipRenderMode::Springs == renderContext.GetDebugShipRenderMode()
        || DebugShipRenderMode::EdgeSprings == renderContext.GetDebugShipRenderMode());

    for (ElementIndex i : *this)
    {
        // Only upload non-deleted springs that are not covered by two super-triangles, unless
        // we are in springs render mode
        if (!mIsDeletedBuffer[i])
        {
            if (IsRope(i) && !doUploadRopesAsSprings)
            {
                renderContext.UploadShipElementRope(
                    shipId,
                    GetEndpointAIndex(i),
                    GetEndpointBIndex(i));
            }
            else if (
                mSuperTrianglesBuffer[i].size() < 2
                || doUploadAllSprings
                || IsRope(i))
            {
                renderContext.UploadShipElementSpring(
                    shipId,
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
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            if (mIsStressedBuffer[i])
            {
                renderContext.UploadShipElementStressedSpring(
                    shipId,
                    GetEndpointAIndex(i),
                    GetEndpointBIndex(i));
            }
        }
    }
}

void Springs::UpdateForStrains(
    GameParameters const & gameParameters,
    Points & points)
{
    float constexpr StrainHighWatermark = 0.5f; // Greater than this multiplier to be stressed
    float constexpr StrainLowWatermark = 0.08f; // Less than this multiplier to become non-stressed

    // Visit all springs
    for (ElementIndex s : *this)
    {
        // Avoid breaking deleted springs and springs with attached bombs
        // (we want to avoid orphanizing bombs)
        if (!mIsDeletedBuffer[s]
            && !mIsBombAttachedBuffer[s])
        {
            // Calculate strain length
            float dx = GetLength(s, points);
            float const strainLength = fabs(mRestLengthBuffer[s] - dx);

            // Check against breaking length
            float const breakingLength = mBreakingLengthBuffer[s];
            if (strainLength > breakingLength)
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
            else if (mIsStressedBuffer[s])
            {
                // Stressed spring...
                // ...see if should un-stress it

                if (strainLength < StrainLowWatermark * breakingLength)
                {
                    // It's not stressed anymore
                    mIsStressedBuffer[s] = false;
                }
            }
            else
            {
                // Not stressed spring
                // ...see if should stress it

                if (strainLength > StrainHighWatermark * breakingLength)
                {
                    // It's stressed!
                    mIsStressedBuffer[s] = true;

                    // Notify stress
                    mGameEventHandler->OnStress(
                        GetBaseStructuralMaterial(s),
                        mParentWorld.IsUnderwater(points.GetPosition(mEndpointsBuffer[s].PointAIndex)),
                        1);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////

void Springs::UpdateForDecayAndTemperatureAndGameParameters(
    float numMechanicalDynamicsIterations,
    float numMechanicalDynamicsIterationsAdjustment,
    float stiffnessAdjustment,
    float dampingAdjustment,
    float strengthAdjustment,
    Points const & points)
{
    float const strengthIterationsAdjustment
        = CalculateSpringStrengthIterationsAdjustment(numMechanicalDynamicsIterationsAdjustment);

    // Recalc all parameters
    for (ElementIndex s : *this)
    {
        if (!IsDeleted(s))
        {
            inline_UpdateForDecayAndTemperatureAndGameParameters(
                s,
                numMechanicalDynamicsIterations,
                stiffnessAdjustment,
                dampingAdjustment,
                strengthAdjustment,
                strengthIterationsAdjustment,
                points);
        }
    }
}

void Springs::UpdateForDecayAndTemperatureAndGameParameters(
    ElementIndex springIndex,
    float numMechanicalDynamicsIterations,
    float stiffnessAdjustment,
    float dampingAdjustment,
    float strengthAdjustment,
    float strengthIterationsAdjustment,
    Points const & points)
{
    inline_UpdateForDecayAndTemperatureAndGameParameters(
        springIndex,
        numMechanicalDynamicsIterations,
        stiffnessAdjustment,
        dampingAdjustment,
        strengthAdjustment,
        strengthIterationsAdjustment,
        points);
}

void Springs::inline_UpdateForDecayAndTemperatureAndGameParameters(
    ElementIndex springIndex,
    float numMechanicalDynamicsIterations,
    float stiffnessAdjustment,
    float dampingAdjustment,
    float strengthAdjustment,
    float strengthIterationsAdjustment,
    Points const & points)
{
    auto const endpointAIndex = GetEndpointAIndex(springIndex);
    auto const endpointBIndex = GetEndpointBIndex(springIndex);

    float const massFactor =
        (points.GetAugmentedMaterialMass(endpointAIndex) * points.GetAugmentedMaterialMass(endpointBIndex))
        / (points.GetAugmentedMaterialMass(endpointAIndex) + points.GetAugmentedMaterialMass(endpointBIndex));

    float const dt = GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations;

    float const springTemperature =
        (points.GetTemperature(endpointAIndex) + points.GetTemperature(endpointBIndex)) / 2.0f;

    float const meltingOverheat = springTemperature - GetMaterialMeltingTemperature(springIndex);

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
    // above the melting point, the lower the stiffness; this is adjusted via a smoothed multiplier with the following
    // edges:
    //  T - Tm <= 0.0 :                 1.0
    //  T - Tm >= DeltaMeltingTMax :    0.0
    //

    float constexpr DeltaMeltingTMax = 1000.0f;
    float const meltingMultiplier = 1.0f - SmoothStep(0.0f, DeltaMeltingTMax, meltingOverheat);

    mCoefficientsBuffer[springIndex].StiffnessCoefficient =
        GameParameters::SpringReductionFraction
        * GetMaterialStiffness(springIndex)
        * stiffnessAdjustment
        * massFactor
        / (dt * dt)
        * meltingMultiplier;


    //
    // Damping coefficient
    //
    // Drag magnitude on the relative velocity component along the spring.
    //

    mCoefficientsBuffer[springIndex].DampingCoefficient =
        GameParameters::SpringDampingCoefficient
        * dampingAdjustment
        * massFactor
        / dt;

    //
    // Breaking length
    //
    // The strength of a spring - i.e. the displacement tolerance or spring breaking point - depends on:
    //  - The material strength and the strength adjustment
    //  - The spring's decay (which itself is a function of the endpoints' decay)
    //  - If the endpoints are melting, their temperature - so to keep springs intact while melting makes them longer
    //  - The actual number of mechanics iterations we'll be performing
    //
    // The strength multiplied with the spring's rest length is the Breaking Length, ready to be
    // compared against the spring absolute delta L
    //

    float const springDecay =
        (points.GetDecay(endpointAIndex) + points.GetDecay(endpointBIndex))
        / 2.0f;

    // TODOHERE: temperature
    mBreakingLengthBuffer[springIndex] =
        GetRestLength(springIndex)
        * GetMaterialStrength(springIndex)
        * strengthAdjustment
        * strengthIterationsAdjustment
        * springDecay;
}

}