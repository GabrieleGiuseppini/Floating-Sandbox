/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "IGameEventHandler.h"
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
        float /*currentSimulationTime*/,
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

    using SuperTrianglesVector = FixedSizeVector<ElementIndex, 2>;

    /*
     * The coefficients used for the spring dynamics.
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
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        GameParameters const & gameParameters)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, mElementCount, true)
        // Endpoints
        , mEndpointsBuffer(mBufferElementCount, mElementCount, Endpoints(NoneElementIndex, NoneElementIndex))
        // Super triangles
        , mSuperTrianglesBuffer(mBufferElementCount, mElementCount, SuperTrianglesVector())
        // Physical
        , mStrengthBuffer(mBufferElementCount, mElementCount, 0.0f)
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
        , mGameEventHandler(std::move(gameEventHandler))
        , mDestroyHandler()
        , mCurrentNumMechanicalDynamicsIterations(gameParameters.NumMechanicalDynamicsIterations<float>())
        , mCurrentStiffnessAdjustment(gameParameters.StiffnessAdjustment)
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

    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        SuperTrianglesVector const & superTriangles,
        Characteristics characteristics,
        Points const & points);

    void Destroy(
        ElementIndex springElementIndex,
        DestroyOptions destroyOptions,
        float currentSimulationTime,
        GameParameters const & gameParameters,
        Points const & points);

    void UpdateGameParameters(
        GameParameters const & gameParameters,
        Points const & points);

    void OnPointMassUpdated(
        ElementIndex springElementIndex,
        Points const & points)
    {
        assert(springElementIndex < mElementCount);

        CalculateStiffnessCoefficient(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            mEndpointsBuffer[springElementIndex].PointBIndex,
            mStiffnessBuffer[springElementIndex],
            mCurrentStiffnessAdjustment,
            mCurrentNumMechanicalDynamicsIterations,
            points);
    }

    /*
     * Calculates the current strain - due to tension or compression - and acts depending on it.
     *
     * Returns true if the spring got broken.
     */
    bool UpdateStrains(
        float currentSimulationTime,
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

    ElementIndex GetPointAIndex(ElementIndex springElementIndex) const
    {
        return mEndpointsBuffer[springElementIndex].PointAIndex;
    }

    ElementIndex GetPointBIndex(ElementIndex springElementIndex) const
    {
        return mEndpointsBuffer[springElementIndex].PointBIndex;
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

    vec2f const & GetPointAPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointAIndex);
    }

    vec2f const & GetPointBPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointBIndex);
    }

    vec2f GetMidpointPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return (GetPointAPosition(springElementIndex, points)
            + GetPointBPosition(springElementIndex, points)) / 2.0f;
    }

    PlaneId GetPlaneId(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        // Return, quite arbitrarily, the plane of point A
        // (the two endpoints might have different plane IDs in case, for example,
        // this spring connects a "string" to a triangle)
        return points.GetPlaneId(GetPointAIndex(springElementIndex));
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

    //
    // Physical
    //

    float GetStrength(ElementIndex springElementIndex) const
    {
        return mStrengthBuffer[springElementIndex];
    }

    float GetStiffness(ElementIndex springElementIndex) const
    {
        return mStiffnessBuffer[springElementIndex];
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

        points.SetMassToStructuralMaterialOffset(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            gameParameters.BombMass,
            *this);

        points.SetMassToStructuralMaterialOffset(
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

        points.SetMassToStructuralMaterialOffset(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            0.0f,
            *this);

        points.SetMassToStructuralMaterialOffset(
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

    // Indexes of the super triangles covering this spring.
    // "Super triangles" are triangles that "cover" this spring when they're rendered - it's either triangles that
    // have this spring as one of their edges, or triangles that (partially) cover this spring
    // (i.e. when this spring is the non-edge diagonal of a two-triangle square).
    // In any case, a spring may have between 0 and at most 2 super triangles.
    Buffer<SuperTrianglesVector> mSuperTrianglesBuffer;

    //
    // Physical
    //

    Buffer<float> mStrengthBuffer;
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
    std::shared_ptr<IGameEventHandler> const mGameEventHandler;

    // The handler registered for spring deletions
    DestroyHandler mDestroyHandler;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentNumMechanicalDynamicsIterations;
    float mCurrentStiffnessAdjustment;

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
