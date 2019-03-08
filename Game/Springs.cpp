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
    SuperTrianglesVector const & superTriangles,
    Characteristics characteristics,
    Points const & points)
{
    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex);

    mSuperTrianglesBuffer.emplace_back(superTriangles);

    // Strength is average
    mStrengthBuffer.emplace_back(
        (points.GetStructuralMaterial(pointAIndex).Strength + points.GetStructuralMaterial(pointBIndex).Strength)
        / 2.0f);

    // Stiffness is average
    float stiffness =
        (points.GetStructuralMaterial(pointAIndex).Stiffness + points.GetStructuralMaterial(pointBIndex).Stiffness)
        / 2.0f;
    mStiffnessBuffer.emplace_back(stiffness);

    mRestLengthBuffer.emplace_back((points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length());

    mCoefficientsBuffer.emplace_back(
        CalculateStiffnessCoefficient(
            pointAIndex,
            pointBIndex,
            stiffness,
            mCurrentSpringStiffnessAdjustment,
            mCurrentNumMechanicalDynamicsIterations,
            points),
        CalculateDampingCoefficient(
            pointAIndex,
            pointBIndex,
            mCurrentSpringDampingAdjustment,
            mCurrentNumMechanicalDynamicsIterations,
            points));

    mCharacteristicsBuffer.emplace_back(characteristics);

    // Base structural material is arbitrarily the weakest of the two;
    // only affects sound and name
    mBaseStructuralMaterialBuffer.emplace_back(
        points.GetStructuralMaterial(pointAIndex).Strength < points.GetStructuralMaterial(pointBIndex).Strength
        ? &points.GetStructuralMaterial(pointAIndex)
        : &points.GetStructuralMaterial(pointBIndex));

    // Spring is impermeable if it's a hull spring (i.e. if at least one endpoint is hull)
    mWaterPermeabilityBuffer.emplace_back(
        Characteristics::None != (characteristics & Characteristics::Hull)
        ? 0.0f
        : 1.0f);

    mIsStressedBuffer.emplace_back(false);

    mIsBombAttachedBuffer.emplace_back(false);
}

void Springs::Destroy(
    ElementIndex springElementIndex,
    DestroyOptions destroyOptions,
    float currentSimulationTime,
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
            currentSimulationTime,
            gameParameters);
    }

    // Fire spring break event, unless told otherwise
    if (!!(destroyOptions & Springs::DestroyOptions::FireBreakEvent))
    {
        mGameEventHandler->OnBreak(
            GetBaseStructuralMaterial(springElementIndex),
            mParentWorld.IsUnderwater(GetPointAPosition(springElementIndex, points)), // Arbitrary
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

void Springs::UpdateGameParameters(
    GameParameters const & gameParameters,
    Points const & points)
{
    float const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<float>();
    if (numMechanicalDynamicsIterations != mCurrentNumMechanicalDynamicsIterations
        || gameParameters.SpringStiffnessAdjustment != mCurrentSpringStiffnessAdjustment
        || gameParameters.SpringDampingAdjustment != mCurrentSpringDampingAdjustment)
    {
        // Recalc coefficients
        for (ElementIndex i : *this)
        {
            if (!IsDeleted(i))
            {
                mCoefficientsBuffer[i].StiffnessCoefficient = CalculateStiffnessCoefficient(
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    GetStiffness(i),
                    gameParameters.SpringStiffnessAdjustment,
                    numMechanicalDynamicsIterations,
                    points);

                mCoefficientsBuffer[i].DampingCoefficient = CalculateDampingCoefficient(
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    gameParameters.SpringDampingAdjustment,
                    numMechanicalDynamicsIterations,
                    points);
            }
        }

        // Remember the new values
        mCurrentNumMechanicalDynamicsIterations = numMechanicalDynamicsIterations;
        mCurrentSpringStiffnessAdjustment = gameParameters.SpringStiffnessAdjustment;
        mCurrentSpringDampingAdjustment = gameParameters.SpringDampingAdjustment;
    }
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
                    GetPointAIndex(i),
                    GetPointBIndex(i));
            }
            else if (
                mSuperTrianglesBuffer[i].size() < 2
                || doUploadAllSprings
                || IsRope(i))
            {
                renderContext.UploadShipElementSpring(
                    shipId,
                    GetPointAIndex(i),
                    GetPointBIndex(i));
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
                    GetPointAIndex(i),
                    GetPointBIndex(i));
            }
        }
    }
}

bool Springs::UpdateStrains(
    float currentSimulationTime,
    GameParameters const & gameParameters,
    Points & points)
{
    // We need to adjust the strength - i.e. the displacement tolerance or spring breaking point - based
    // on the actual number of mechanics iterations we'll be performing.
    //
    // After one iteration the spring displacement dL is reduced to:
    //  dL * (1-SRF)
    // where SRF is the value of the SpringReductionFraction parameter. After N iterations this would be:
    //  dL * (1-SRF)^N
    //
    // This formula suggests a simple exponential relationship, but empirical data (e.g. explosions on the Titanic)
    // suggests the following relationship:
    //
    //  s' = s * 4 / (1 + 3*(R^1.3))
    //
    // Where R is the N'/N ratio.

    float const iterationsAdjustmentFactor =
        4.0f
        / (1.0f + 3.0f * pow(gameParameters.NumMechanicalDynamicsIterationsAdjustment, 1.3f));

    float const effectiveStrengthAdjustment =
        iterationsAdjustmentFactor
        * gameParameters.SpringStrengthAdjustment;

    // Flag remembering whether at least one spring broke
    bool isAtLeastOneBroken = false;

    // Visit all springs
    for (ElementIndex s : *this)
    {
        // Avoid breaking deleted springs and springs with attached bombs
        // (we want to avoid orphanizing bombs)
        if (!mIsDeletedBuffer[s]
            && !mIsBombAttachedBuffer[s])
        {
            // Calculate strain
            float dx = (points.GetPosition(mEndpointsBuffer[s].PointAIndex) - points.GetPosition(mEndpointsBuffer[s].PointBIndex)).length();
            float const strain = fabs(mRestLengthBuffer[s] - dx) / mRestLengthBuffer[s];

            // Check against strength
            float const effectiveStrength = effectiveStrengthAdjustment * mStrengthBuffer[s];
            if (strain > effectiveStrength)
            {
                // It's broken!

                // Destroy this spring
                this->Destroy(
                    s,
                    DestroyOptions::FireBreakEvent // Notify Break
                    | DestroyOptions::DestroyAllTriangles,
                    currentSimulationTime,
                    gameParameters,
                    points);

                isAtLeastOneBroken = true;
            }
            else if (strain > 0.5f * effectiveStrength)
            {
                // It's stressed!
                if (!mIsStressedBuffer[s])
                {
                    mIsStressedBuffer[s] = true;

                    // Notify stress
                    mGameEventHandler->OnStress(
                        GetBaseStructuralMaterial(s),
                        mParentWorld.IsUnderwater(points.GetPosition(mEndpointsBuffer[s].PointAIndex)),
                        1);
                }
            }
            else
            {
                // Just fine
                mIsStressedBuffer[s] = false;
            }
        }
    }

    return isAtLeastOneBroken;
}

float Springs::CalculateStiffnessCoefficient(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    float springStiffness,
    float stiffnessAdjustment,
    float numMechanicalDynamicsIterations,
    Points const & points)
{
    //
    // The "stiffness coefficient" is the factor which, once multiplied with the spring displacement,
    // yields the spring force, according to Hooke's law.
    //
    // We calculate the coefficient so that the two forces applied to each of the two masses produce a resulting
    // change in position equal to a fraction SpringReductionFraction * adjustment of the spring displacement,
    // in the time interval of a single mechanical dynamics simulation.
    //
    // The adjustment is both the material-specific adjustment and the global game adjustment.
    //

    float const massFactor =
        (points.GetMass(pointAIndex) * points.GetMass(pointBIndex))
        / (points.GetMass(pointAIndex) + points.GetMass(pointBIndex));

    float const dtSquared =
        (GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations)
        * (GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations);

    return GameParameters::SpringReductionFraction
        * springStiffness
        * stiffnessAdjustment
        * massFactor
        / dtSquared;
}

float Springs::CalculateDampingCoefficient(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    float dampingAdjustment,
    float numMechanicalDynamicsIterations,
    Points const & points)
{
    float const massFactor =
        (points.GetMass(pointAIndex) * points.GetMass(pointBIndex))
        / (points.GetMass(pointAIndex) + points.GetMass(pointBIndex));

    float const dt = GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations;

    return GameParameters::SpringDampingCoefficient
        * dampingAdjustment
        * massFactor
        / dt;
}

}