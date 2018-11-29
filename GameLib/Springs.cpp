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
    FixedSizeVector<ElementIndex, 2u> const & superTriangles,
    Characteristics characteristics,
    Points const & points)
{
    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex);

    mSuperTrianglesBuffer.emplace_back(superTriangles);

    // Strength is average
    mStrengthBuffer.emplace_back((points.GetMaterial(pointAIndex)->Strength + points.GetMaterial(pointBIndex)->Strength) / 2.0f);

    // Stiffness is average
    float stiffness = (points.GetMaterial(pointAIndex)->Stiffness + points.GetMaterial(pointBIndex)->Stiffness) / 2.0f;
    mStiffnessBuffer.emplace_back(stiffness);

    mRestLengthBuffer.emplace_back((points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length());
    mCoefficientsBuffer.emplace_back(
        CalculateStiffnessCoefficient(
            pointAIndex,
            pointBIndex,
            stiffness,
            mCurrentStiffnessAdjustment,
            mCurrentNumMechanicalDynamicsIterations,
            points),
        CalculateDampingCoefficient(
            pointAIndex,
            pointBIndex,
            mCurrentNumMechanicalDynamicsIterations,
            points));
    mCharacteristicsBuffer.emplace_back(characteristics);

    // Base material is arbitrarily the weakest of the two;
    // only affects sound and name
    mBaseMaterialBuffer.emplace_back(
        points.GetMaterial(pointAIndex)->Strength < points.GetMaterial(pointBIndex)->Strength
        ? points.GetMaterial(pointAIndex)
        : points.GetMaterial(pointBIndex));

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
            GetBaseMaterial(springElementIndex),
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
        || gameParameters.StiffnessAdjustment != mCurrentStiffnessAdjustment)
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
                    gameParameters.StiffnessAdjustment,
                    numMechanicalDynamicsIterations,
                    points);

                mCoefficientsBuffer[i].DampingCoefficient = CalculateDampingCoefficient(
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    numMechanicalDynamicsIterations,
                    points);
            }
        }

        // Remember the new values
        mCurrentNumMechanicalDynamicsIterations = numMechanicalDynamicsIterations;
        mCurrentStiffnessAdjustment = gameParameters.StiffnessAdjustment;
    }
}

void Springs::UploadElements(
    ShipId shipId,
    Render::RenderContext & renderContext,
    Points const & points) const
{
    bool const doUploadAllSprings = (ShipRenderMode::Springs == renderContext.GetShipRenderMode());

    for (ElementIndex i : *this)
    {
        // Only upload non-deleted springs that are not covered by two super-triangles, unless
        // we are in springs render mode
        if (!mIsDeletedBuffer[i])
        {
            assert(points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointBIndex(i)));

            if (IsRope(i))
            {
                renderContext.UploadShipElementRope(
                    shipId,
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    points.GetConnectedComponentId(GetPointAIndex(i)));
            }
            else if (mSuperTrianglesBuffer[i].size() < 2 || doUploadAllSprings)
            {
                renderContext.UploadShipElementSpring(
                    shipId,
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    points.GetConnectedComponentId(GetPointAIndex(i)));
            }
        }
    }
}

void Springs::UploadStressedSpringElements(
    ShipId shipId,
    Render::RenderContext & renderContext,
    Points const & points) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            if (mIsStressedBuffer[i])
            {
                assert(points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointBIndex(i)));

                renderContext.UploadShipElementStressedSpring(
                    shipId,
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    points.GetConnectedComponentId(GetPointAIndex(i)));
            }
        }
    }
}

bool Springs::UpdateStrains(
    float currentSimulationTime,
    GameParameters const & gameParameters,
    Points & points)
{
    bool isAtLeastOneBroken = false;

    for (ElementIndex i : *this)
    {
        // Avoid breaking deleted springs
        if (!mIsDeletedBuffer[i])
        {
            // Calculate strain
            float dx = (points.GetPosition(mEndpointsBuffer[i].PointAIndex) - points.GetPosition(mEndpointsBuffer[i].PointBIndex)).length();
            float const strain = fabs(mRestLengthBuffer[i] - dx) / mRestLengthBuffer[i];

            // Check against strength
            float const effectiveStrength = gameParameters.StrengthAdjustment * mStrengthBuffer[i];
            if (strain > effectiveStrength)
            {
                // It's broken!

                // Destroy this spring
                this->Destroy(
                    i,
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
                if (!mIsStressedBuffer[i])
                {
                    mIsStressedBuffer[i] = true;

                    // Notify stress
                    mGameEventHandler->OnStress(
                        mBaseMaterialBuffer[i],
                        mParentWorld.IsUnderwater(points.GetPosition(mEndpointsBuffer[i].PointAIndex)),
                        1);
                }
            }
            else
            {
                // Just fine
                mIsStressedBuffer[i] = false;
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
    // We calculate the coefficient so that the two forces applied to each of the masses reduce the spring
    // displacement by a quantity equal to C * adjustment, in the time interval of the dynamics simulation.
    //
    // The adjustment is both the material-specific adjustment and the global game adjustment.
    //

    static constexpr float C = 0.4f;

    float const massFactor =
        (points.GetMass(pointAIndex) * points.GetMass(pointBIndex))
        / (points.GetMass(pointAIndex) + points.GetMass(pointBIndex));

    float const dtSquared =
        (GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations)
        * (GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations);

    return C * springStiffness * stiffnessAdjustment * massFactor / dtSquared;
}

float Springs::CalculateDampingCoefficient(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    float numMechanicalDynamicsIterations,
    Points const & points)
{
    // The empirically-determined constant for the spring damping
    //
    // The simulation is quite sensitive to this value:
    // - 0.03 is almost fine (though bodies are sometimes soft)
    // - 0.8 makes everything explode
    static constexpr float C = 0.03f;

    float const massFactor =
        (points.GetMass(pointAIndex) * points.GetMass(pointBIndex))
        / (points.GetMass(pointAIndex) + points.GetMass(pointBIndex));

    float const dt = GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations;

    return C * massFactor / dt;
}

}