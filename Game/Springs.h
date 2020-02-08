/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "Materials.h"
#include "RenderContext.h"

#include <GameCore/Buffer.h>
#include <GameCore/BufferAllocator.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/EnumFlags.h>
#include <GameCore/FixedSizeVector.h>

#include <cassert>
#include <functional>
#include <limits>

namespace Physics
{

class Springs : public ElementContainer
{
public:

    enum class DestroyOptions
    {
        DoNotFireBreakEvent = 0,
        FireBreakEvent = 1,

        DestroyOnlyConnectedTriangle = 0,
        DestroyAllTriangles = 2
    };

    enum class Characteristics : uint8_t
    {
        None = 0,
        Hull = 1,    // Does not take water
        Rope = 2     // Ropes are drawn differently
    };

    /*
     * The endpoints of a spring.
     */
    struct Endpoints
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;

        Endpoints(
            ElementIndex pointAIndex,
            ElementIndex pointBIndex)
            : PointAIndex(pointAIndex)
            , PointBIndex(pointBIndex)
        {}
    };

    /*
     * The pre-calculated coefficients used for the spring dynamics.
     */
    struct Coefficients
    {
        float StiffnessCoefficient;
        float DampingCoefficient;

        Coefficients(
            float stiffnessCoefficient,
            float dampingCoefficient)
            : StiffnessCoefficient(stiffnessCoefficient)
            , DampingCoefficient(dampingCoefficient)
        {}
    };

private:

    /*
     * The factory angle of the spring from the point of view
     * of each endpoint.
     *
     * Angle 0 is E, angle 1 is SE, ..., angle 7 is NE.
     */
    struct EndpointOctants
    {
        int32_t PointAOctant;
        int32_t PointBOctant;

        EndpointOctants(
            int32_t pointAOctant,
            int32_t pointBOctant)
            : PointAOctant(pointAOctant)
            , PointBOctant(pointBOctant)
        {}
    };

    /*
     * The triangles that have an edge along this spring.
     */
    using SuperTrianglesVector = FixedSizeVector<ElementIndex, 2>;

public:

    Springs(
        ElementCount elementCount,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, mElementCount, true)
        // Endpoints
        , mEndpointsBuffer(mBufferElementCount, mElementCount, Endpoints(NoneElementIndex, NoneElementIndex))
        // Factory endpoint octants
        , mFactoryEndpointOctantsBuffer(mBufferElementCount, mElementCount, EndpointOctants(0, 4))
        // Super triangles
        , mSuperTrianglesBuffer(mBufferElementCount, mElementCount, SuperTrianglesVector())
        , mFactorySuperTrianglesBuffer(mBufferElementCount, mElementCount, SuperTrianglesVector())
        // Physical
        , mMaterialStrengthBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mBreakingElongationBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mMaterialStiffnessBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mFactoryRestLengthBuffer(mBufferElementCount, mElementCount, 1.0f)
        , mRestLengthBuffer(mBufferElementCount, mElementCount, 1.0f)
        , mCoefficientsBuffer(mBufferElementCount, mElementCount, Coefficients(0.0f, 0.0f))
        , mMaterialCharacteristicsBuffer(mBufferElementCount, mElementCount, Characteristics::None)
        , mBaseStructuralMaterialBuffer(mBufferElementCount, mElementCount, nullptr)
        // Water
        , mMaterialWaterPermeabilityBuffer(mBufferElementCount, mElementCount, 0.0f)
        // Heat
        , mMaterialThermalConductivityBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mMaterialMeltingTemperatureBuffer(mBufferElementCount, mElementCount, 0.0f)
        // Stress
        , mIsStressedBuffer(mBufferElementCount, mElementCount, false)
        // Bombs
        , mIsBombAttachedBuffer(mBufferElementCount, mElementCount, false)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mShipPhysicsHandler(nullptr)
        , mCurrentNumMechanicalDynamicsIterations(gameParameters.NumMechanicalDynamicsIterations<float>())
        , mCurrentNumMechanicalDynamicsIterationsAdjustment(gameParameters.NumMechanicalDynamicsIterationsAdjustment)
        , mCurrentSpringStiffnessAdjustment(gameParameters.SpringStiffnessAdjustment)
        , mCurrentSpringDampingAdjustment(gameParameters.SpringDampingAdjustment)
        , mCurrentSpringStrengthAdjustment(gameParameters.SpringStrengthAdjustment)
        , mFloatBufferAllocator(mBufferElementCount)
        , mVec2fBufferAllocator(mBufferElementCount)
    {
    }

    Springs(Springs && other) = default;

    void RegisterShipPhysicsHandler(IShipPhysicsHandler * shipPhysicsHandler)
    {
        mShipPhysicsHandler = shipPhysicsHandler;
    }

    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        int32_t factoryPointAOctant,
        int32_t factoryPointBOctant,
        SuperTrianglesVector const & superTriangles,
        Characteristics characteristics,
        Points const & points);

    void Destroy(
        ElementIndex springElementIndex,
        DestroyOptions destroyOptions,
        GameParameters const & gameParameters,
        Points const & points);

    void Restore(
        ElementIndex springElementIndex,
        GameParameters const & gameParameters,
        Points const & points);

    void UpdateForGameParameters(
        GameParameters const & gameParameters,
        Points const & points);

    void UpdateForDecayAndTemperatureAndGameParameters(
        GameParameters const & gameParameters,
        Points const & points);

    void UpdateForRestLength(
        ElementIndex springElementIndex,
        Points const & points)
    {
        // Recalculate parameters for this spring
        UpdateForDecayAndTemperatureAndGameParameters(
            springElementIndex,
            mCurrentNumMechanicalDynamicsIterations,
            mCurrentSpringStiffnessAdjustment,
            mCurrentSpringDampingAdjustment,
            mCurrentSpringStrengthAdjustment,
            CalculateSpringStrengthIterationsAdjustment(mCurrentNumMechanicalDynamicsIterationsAdjustment),
            points);
    }

    void UpdateForMass(
        ElementIndex springElementIndex,
        Points const & points)
    {
        // Recalculate parameters for this spring
        UpdateForDecayAndTemperatureAndGameParameters(
            springElementIndex,
            mCurrentNumMechanicalDynamicsIterations,
            mCurrentSpringStiffnessAdjustment,
            mCurrentSpringDampingAdjustment,
            mCurrentSpringStrengthAdjustment,
            CalculateSpringStrengthIterationsAdjustment(mCurrentNumMechanicalDynamicsIterationsAdjustment),
            points);
    }

    /*
     * Calculates the current strain - due to tension or compression - and acts depending on it,
     * eventually breaking springs.
     */
    void UpdateForStrains(
        GameParameters const & gameParameters,
        Points & points);

    //
    // Render
    //

    void UploadElements(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

    void UploadStressedSpringElements(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

public:

    //
    // IsDeleted
    //

    bool IsDeleted(ElementIndex springElementIndex) const
    {
        return mIsDeletedBuffer[springElementIndex];
    }

    //
    // Endpoints
    //

    ElementIndex GetEndpointAIndex(ElementIndex springElementIndex) const noexcept
    {
        return mEndpointsBuffer[springElementIndex].PointAIndex;
    }

    ElementIndex GetEndpointBIndex(ElementIndex springElementIndex) const noexcept
    {
        return mEndpointsBuffer[springElementIndex].PointBIndex;
    }

    ElementIndex GetOtherEndpointIndex(
        ElementIndex springElementIndex,
        ElementIndex pointElementIndex) const
    {
        if (pointElementIndex == mEndpointsBuffer[springElementIndex].PointAIndex)
            return mEndpointsBuffer[springElementIndex].PointBIndex;
        else
        {
            assert(pointElementIndex == mEndpointsBuffer[springElementIndex].PointBIndex);
            return mEndpointsBuffer[springElementIndex].PointAIndex;
        }
    }

    Endpoints const * restrict GetEndpointsBuffer() const noexcept
    {
        return mEndpointsBuffer.data();
    }

    // Returns +1.0 if the spring is directed outward from the specified point;
    // otherwise, -1.0.
    float GetSpringDirectionFrom(
        ElementIndex springElementIndex,
        ElementIndex pointIndex) const
    {
        if (pointIndex == mEndpointsBuffer[springElementIndex].PointAIndex)
            return 1.0f;
        else
            return -1.0f;
    }

    vec2f const & GetEndpointAPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointAIndex);
    }

    vec2f const & GetEndpointBPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointBIndex);
    }

    vec2f GetMidpointPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return (GetEndpointAPosition(springElementIndex, points)
            + GetEndpointBPosition(springElementIndex, points)) / 2.0f;
    }

    PlaneId GetPlaneId(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        // Return, quite arbitrarily, the plane of point A
        // (the two endpoints might have different plane IDs in case, for example,
        // this spring connects a "string" to a triangle)
        return points.GetPlaneId(GetEndpointAIndex(springElementIndex));
    }

    //
    // Factory endpoint octants
    //

    int32_t GetFactoryEndpointAOctant(ElementIndex springElementIndex) const
    {
        return mFactoryEndpointOctantsBuffer[springElementIndex].PointAOctant;
    }

    int32_t GetFactoryEndpointBOctant(ElementIndex springElementIndex) const
    {
        return mFactoryEndpointOctantsBuffer[springElementIndex].PointBOctant;
    }

    int32_t GetFactoryEndpointOctant(
        ElementIndex springElementIndex,
        ElementIndex pointElementIndex) const
    {
        if (pointElementIndex == GetEndpointAIndex(springElementIndex))
            return GetFactoryEndpointAOctant(springElementIndex);
        else
        {
            assert(pointElementIndex == GetEndpointBIndex(springElementIndex));
            return GetFactoryEndpointBOctant(springElementIndex);
        }
    }

    int32_t GetFactoryOtherEndpointOctant(
        ElementIndex springElementIndex,
        ElementIndex pointElementIndex) const
    {
        if (pointElementIndex == GetEndpointAIndex(springElementIndex))
            return GetFactoryEndpointBOctant(springElementIndex);
        else
        {
            assert(pointElementIndex == GetEndpointBIndex(springElementIndex));
            return GetFactoryEndpointAOctant(springElementIndex);
        }
    }

    //
    // Super triangles
    //

    inline auto const & GetSuperTriangles(ElementIndex springElementIndex) const
    {
        return mSuperTrianglesBuffer[springElementIndex];
    }

    inline void AddSuperTriangle(
        ElementIndex springElementIndex,
        ElementIndex superTriangleElementIndex)
    {
        assert(mFactorySuperTrianglesBuffer[springElementIndex].contains(
            [superTriangleElementIndex](auto st)
            {
                return st = superTriangleElementIndex;
            }));

        mSuperTrianglesBuffer[springElementIndex].push_back(superTriangleElementIndex);
    }

    inline void RemoveSuperTriangle(
        ElementIndex springElementIndex,
        ElementIndex superTriangleElementIndex)
    {
        bool found = mSuperTrianglesBuffer[springElementIndex].erase_first(superTriangleElementIndex);

        assert(found);
        (void)found;
    }

    inline void ClearSuperTriangles(ElementIndex springElementIndex)
    {
        mSuperTrianglesBuffer[springElementIndex].clear();
    }

    auto const & GetFactorySuperTriangles(ElementIndex springElementIndex) const
    {
        return mFactorySuperTrianglesBuffer[springElementIndex];
    }

    void RestoreFactorySuperTriangles(ElementIndex springElementIndex)
    {
        assert(mSuperTrianglesBuffer[springElementIndex].empty());

        std::copy(
            mFactorySuperTrianglesBuffer[springElementIndex].cbegin(),
            mFactorySuperTrianglesBuffer[springElementIndex].cend(),
            mSuperTrianglesBuffer[springElementIndex].begin());
    }

    //
    // Physical
    //

    float GetMaterialStrength(ElementIndex springElementIndex) const
    {
        return mMaterialStrengthBuffer[springElementIndex];
    }

    float GetMaterialStiffness(ElementIndex springElementIndex) const
    {
        return mMaterialStiffnessBuffer[springElementIndex];
    }

    float GetLength(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return
            (points.GetPosition(GetEndpointAIndex(springElementIndex)) - points.GetPosition(GetEndpointBIndex(springElementIndex)))
            .length();
    }

    float GetFactoryRestLength(ElementIndex springElementIndex) const
    {
        return mFactoryRestLengthBuffer[springElementIndex];
    }

    float GetRestLength(ElementIndex springElementIndex) const noexcept
    {
        return mRestLengthBuffer[springElementIndex];
    }

    float const * restrict GetRestLengthBuffer() const noexcept
    {
        return mRestLengthBuffer.data();
    }

    void SetRestLength(
        ElementIndex springElementIndex,
        float restLength)
    {
        mRestLengthBuffer[springElementIndex] = restLength;
    }

    float GetStiffnessCoefficient(ElementIndex springElementIndex) const noexcept
    {
        return mCoefficientsBuffer[springElementIndex].StiffnessCoefficient;
    }

    float GetDampingCoefficient(ElementIndex springElementIndex) const noexcept
    {
        return mCoefficientsBuffer[springElementIndex].DampingCoefficient;
    }

    Coefficients const * restrict GetCoefficientsBuffer() const noexcept
    {
        return mCoefficientsBuffer.data();
    }

    StructuralMaterial const & GetBaseStructuralMaterial(ElementIndex springElementIndex) const
    {
        // If this method is invoked, this is not a placeholder
        assert(nullptr != mBaseStructuralMaterialBuffer[springElementIndex]);
        return *(mBaseStructuralMaterialBuffer[springElementIndex]);
    }

    inline bool IsRope(ElementIndex springElementIndex) const;

    //
    // Water
    //

    float GetMaterialWaterPermeability(ElementIndex springElementIndex) const
    {
        return mMaterialWaterPermeabilityBuffer[springElementIndex];
    }

    //
    // Heat
    //

    float GetMaterialThermalConductivity(ElementIndex springElementIndex) const
    {
        return mMaterialThermalConductivityBuffer[springElementIndex];
    }

    float GetMaterialMeltingTemperature(ElementIndex springElementIndex) const
    {
        return mMaterialMeltingTemperatureBuffer[springElementIndex];
    }

    //
    // Bombs
    //

    bool IsBombAttached(ElementIndex springElementIndex) const
    {
        return mIsBombAttachedBuffer[springElementIndex];
    }

    void AttachBomb(
        ElementIndex springElementIndex,
        Points & points,
        GameParameters const & gameParameters)
    {
        assert(false == mIsDeletedBuffer[springElementIndex]);
        assert(false == mIsBombAttachedBuffer[springElementIndex]);

        mIsBombAttachedBuffer[springElementIndex] = true;

        // Augment mass of endpoints due to bomb

        points.AugmentMaterialMass(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            gameParameters.BombMass,
            *this);

        points.AugmentMaterialMass(
            mEndpointsBuffer[springElementIndex].PointBIndex,
            gameParameters.BombMass,
            *this);
    }

    void DetachBomb(
        ElementIndex springElementIndex,
        Points & points)
    {
        assert(false == mIsDeletedBuffer[springElementIndex]);
        assert(true == mIsBombAttachedBuffer[springElementIndex]);

        mIsBombAttachedBuffer[springElementIndex] = false;

        // Reset mass of endpoints

        points.AugmentMaterialMass(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            0.0f,
            *this);

        points.AugmentMaterialMass(
            mEndpointsBuffer[springElementIndex].PointBIndex,
            0.0f,
            *this);
    }

    //
    // Temporary buffer
    //

    std::shared_ptr<Buffer<float>> AllocateWorkBufferFloat()
    {
        return mFloatBufferAllocator.Allocate();
    }

    std::shared_ptr<Buffer<vec2f>> AllocateWorkBufferVec2f()
    {
        return mVec2fBufferAllocator.Allocate();
    }

private:

    static float CalculateSpringStrengthIterationsAdjustment(float numMechanicalDynamicsIterationsAdjustment)
    {
        // We need to adjust the strength - i.e. the displacement tolerance or spring breaking point - based
        // on the actual number of mechanics iterations we'll be performing.
        //
        // After one iteration the spring displacement dL = L - L0 is reduced to:
        //  dL * (1-SRF)
        // where SRF is the value of the SpringReductionFraction parameter. After N iterations this would be:
        //  dL * (1-SRF)^N
        //
        // This formula suggests a simple exponential relationship, but empirical data (e.g. explosions on the Titanic)
        // suggest the following relationship:
        //
        //  s' = s * 4 / (1 + 3*(R^1.3))
        //
        // Where R is the N'/N ratio.

        return
            4.0f
            / (1.0f + 3.0f * pow(numMechanicalDynamicsIterationsAdjustment, 1.3f));
    }

    void UpdateForDecayAndTemperatureAndGameParameters(
        float numMechanicalDynamicsIterations,
        float numMechanicalDynamicsIterationsAdjustment,
        float stiffnessAdjustment,
        float dampingAdjustment,
        float strengthAdjustment,
        Points const & points);

    void UpdateForDecayAndTemperatureAndGameParameters(
        ElementIndex springIndex,
        float numMechanicalDynamicsIterations,
        float stiffnessAdjustment,
        float dampingAdjustment,
        float strengthAdjustment,
        float strengthIterationsAdjustment,
        Points const & points);

    inline void inline_UpdateForDecayAndTemperatureAndGameParameters(
        ElementIndex springIndex,
        float numMechanicalDynamicsIterations,
        float stiffnessAdjustment,
        float dampingAdjustment,
        float strengthAdjustment,
        float strengthIterationsAdjustment,
        Points const & points);

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Endpoints
    Buffer<Endpoints> mEndpointsBuffer;

    // Factory-time endpoint octants
    Buffer<EndpointOctants> mFactoryEndpointOctantsBuffer;

    // Indexes of the super triangles covering this spring.
    // "Super triangles" are triangles that "cover" this spring when they're rendered - it's either triangles that
    // have this spring as one of their edges, or triangles that (partially) cover this spring
    // (i.e. when this spring is the non-edge diagonal of a two-triangle square).
    // In any case, a spring may have between 0 and at most 2 super triangles.
    Buffer<SuperTrianglesVector> mSuperTrianglesBuffer;
    Buffer<SuperTrianglesVector> mFactorySuperTrianglesBuffer;

    //
    // Physical
    //

    Buffer<float> mMaterialStrengthBuffer;
    Buffer<float> mBreakingElongationBuffer;
    Buffer<float> mMaterialStiffnessBuffer;
    Buffer<float> mFactoryRestLengthBuffer;
    Buffer<float> mRestLengthBuffer;
    Buffer<Coefficients> mCoefficientsBuffer;
    Buffer<Characteristics> mMaterialCharacteristicsBuffer;
    Buffer<StructuralMaterial const *> mBaseStructuralMaterialBuffer;

    //
    // Water
    //

    // Water propagates through this spring according to this value;
    // 0.0 makes water not propagate
    Buffer<float> mMaterialWaterPermeabilityBuffer;

    //
    // Hear
    //

    Buffer<float> mMaterialThermalConductivityBuffer;
    Buffer<float> mMaterialMeltingTemperatureBuffer;

    //
    // Stress
    //

    // State variable that tracks when we enter and exit the stressed state
    Buffer<bool> mIsStressedBuffer;

    //
    // Bombs
    //

    Buffer<bool> mIsBombAttachedBuffer;

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    World & mParentWorld;
    std::shared_ptr<GameEventDispatcher> const mGameEventHandler;
    IShipPhysicsHandler * mShipPhysicsHandler;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentNumMechanicalDynamicsIterations;
    float mCurrentNumMechanicalDynamicsIterationsAdjustment;
    float mCurrentSpringStiffnessAdjustment;
    float mCurrentSpringDampingAdjustment;
    float mCurrentSpringStrengthAdjustment;

    // Allocators for work buffers
    BufferAllocator<float> mFloatBufferAllocator;
    BufferAllocator<vec2f> mVec2fBufferAllocator;
};

}

template <> struct is_flag<Physics::Springs::DestroyOptions> : std::true_type {};
template <> struct is_flag<Physics::Springs::Characteristics> : std::true_type {};

inline bool Physics::Springs::IsRope(ElementIndex springElementIndex) const
{
    assert(springElementIndex < mElementCount);

    return !!(mMaterialCharacteristicsBuffer[springElementIndex] & Physics::Springs::Characteristics::Rope);
}
