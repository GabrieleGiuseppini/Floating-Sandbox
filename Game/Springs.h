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

    using DestroyHandler = std::function<void(
        ElementIndex,
        bool /*destroyTriangles*/,
        GameParameters const &)>;

    using RestoreHandler = std::function<void(
        ElementIndex,
        GameParameters const &)>;

private:

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
        , mStrengthBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mMaterialStrengthBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mStiffnessBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mRestLengthBuffer(mBufferElementCount, mElementCount, 1.0f)
        , mCoefficientsBuffer(mBufferElementCount, mElementCount, Coefficients(0.0f, 0.0f))
        , mCharacteristicsBuffer(mBufferElementCount, mElementCount, Characteristics::None)
        , mBaseStructuralMaterialBuffer(mBufferElementCount, mElementCount, nullptr)
        // Water
        , mWaterPermeabilityBuffer(mBufferElementCount, mElementCount, 0.0f)
        // Stress
        , mIsStressedBuffer(mBufferElementCount, mElementCount, false)
        // Bombs
        , mIsBombAttachedBuffer(mBufferElementCount, mElementCount, false)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mDestroyHandler()
        , mRestoreHandler()
        , mCurrentNumMechanicalDynamicsIterations(gameParameters.NumMechanicalDynamicsIterations<float>())
        , mCurrentSpringStiffnessAdjustment(gameParameters.SpringStiffnessAdjustment)
        , mCurrentSpringDampingAdjustment(gameParameters.SpringDampingAdjustment)
        , mFloatBufferAllocator(mBufferElementCount)
        , mVec2fBufferAllocator(mBufferElementCount)
    {
    }

    Springs(Springs && other) = default;

    /*
     * Sets a (single) handler that is invoked whenever a spring is destroyed.
     *
     * The handler is invoked right before the spring is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted spring might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other springs from it is not supported
     * and leads to undefined behavior.
     *
     * Setting more than one handler is not supported and leads to undefined behavior.
     */
    void RegisterDestroyHandler(DestroyHandler destroyHandler)
    {
        assert(!mDestroyHandler);
        mDestroyHandler = std::move(destroyHandler);
    }

    /*
     * Sets a (single) handler that is invoked whenever a spring is restored.
     *
     * The handler is invoked right after the spring is unmarked as deleted. However,
     * other elements connected to the soon-to-be-deleted spring might not yet have been
     * restored.
     *
     * The handler is not re-entrant: restoring other springs from it is not supported
     * and leads to undefined behavior.
     *
     * Setting more than one handler is not supported and leads to undefined behavior.
     */
    void RegisterRestoreHandler(RestoreHandler restoreHandler)
    {
        assert(!mRestoreHandler);
        mRestoreHandler = std::move(restoreHandler);
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

    void UpdateGameParameters(
        GameParameters const & gameParameters,
        Points const & points);

    void OnEndpointMassUpdated(
        ElementIndex springElementIndex,
        Points const & points)
    {
        assert(springElementIndex < mElementCount);

        mCoefficientsBuffer[springElementIndex].StiffnessCoefficient =
            CalculateStiffnessCoefficient(
                mEndpointsBuffer[springElementIndex].PointAIndex,
                mEndpointsBuffer[springElementIndex].PointBIndex,
                mStiffnessBuffer[springElementIndex],
                mCurrentSpringStiffnessAdjustment,
                mCurrentNumMechanicalDynamicsIterations,
                points);

        mCoefficientsBuffer[springElementIndex].DampingCoefficient =
            CalculateDampingCoefficient(
                mEndpointsBuffer[springElementIndex].PointAIndex,
                mEndpointsBuffer[springElementIndex].PointBIndex,
                mCurrentSpringDampingAdjustment,
                mCurrentNumMechanicalDynamicsIterations,
                points);
    }

    /*
     * Calculates the current strain - due to tension or compression - and acts depending on it.
     *
     * Returns true if the spring got broken.
     */
    bool UpdateStrains(
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

    ElementIndex GetEndpointAIndex(ElementIndex springElementIndex) const
    {
        return mEndpointsBuffer[springElementIndex].PointAIndex;
    }

    ElementIndex GetEndpointBIndex(ElementIndex springElementIndex) const
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

    float GetStrength(ElementIndex springElementIndex) const
    {
        return mStrengthBuffer[springElementIndex];
    }

    void SetStrength(
        ElementIndex springElementIndex,
        float value)
    {
        mStrengthBuffer[springElementIndex] = value;
    }

    float GetMaterialStrength(ElementIndex springElementIndex) const
    {
        return mMaterialStrengthBuffer[springElementIndex];
    }

    float GetStiffness(ElementIndex springElementIndex) const
    {
        return mStiffnessBuffer[springElementIndex];
    }

    float GetLength(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return
            (points.GetPosition(GetEndpointAIndex(springElementIndex)) - points.GetPosition(GetEndpointBIndex(springElementIndex)))
            .length();
    }

    float GetRestLength(ElementIndex springElementIndex) const
    {
        return mRestLengthBuffer[springElementIndex];
    }

    float GetStiffnessCoefficient(ElementIndex springElementIndex) const
    {
        return mCoefficientsBuffer[springElementIndex].StiffnessCoefficient;
    }

    float GetDampingCoefficient(ElementIndex springElementIndex) const
    {
        return mCoefficientsBuffer[springElementIndex].DampingCoefficient;
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

    float GetWaterPermeability(ElementIndex springElementIndex) const
    {
        return mWaterPermeabilityBuffer[springElementIndex];
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

        points.AugmentStructuralMass(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            gameParameters.BombMass,
            *this);

        points.AugmentStructuralMass(
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

        points.AugmentStructuralMass(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            0.0f,
            *this);

        points.AugmentStructuralMass(
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

    static float CalculateStiffnessCoefficient(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        float springStiffness,
        float stiffnessAdjustment,
        float numMechanicalDynamicsIterations,
        Points const & points);

    static float CalculateDampingCoefficient(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        float dampingAdjustment,
        float numMechanicalDynamicsIterations,
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

    Buffer<float> mStrengthBuffer; // Decayed strength
    Buffer<float> mMaterialStrengthBuffer; // Original strength
    Buffer<float> mStiffnessBuffer;
    Buffer<float> mRestLengthBuffer;
    Buffer<Coefficients> mCoefficientsBuffer;
    Buffer<Characteristics> mCharacteristicsBuffer;
    Buffer<StructuralMaterial const *> mBaseStructuralMaterialBuffer;

    //
    // Water
    //

    // Water propagates through this spring according to this value;
    // 0.0 makes water not propagate
    Buffer<float> mWaterPermeabilityBuffer;

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

    // The handler registered for spring deletions
    DestroyHandler mDestroyHandler;

    // The handler registered for spring restores
    RestoreHandler mRestoreHandler;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentNumMechanicalDynamicsIterations;
    float mCurrentSpringStiffnessAdjustment;
    float mCurrentSpringDampingAdjustment;

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

    return !!(mCharacteristicsBuffer[springElementIndex] & Physics::Springs::Characteristics::Rope);
}
