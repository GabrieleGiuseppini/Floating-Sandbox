/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

#include <GameCore/Buffer.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/FixedSizeVector.h>

#include <algorithm>
#include <cassert>
#include <functional>

namespace Physics
{

class Triangles : public ElementContainer
{
public:

    using DestroyHandler = std::function<void(ElementIndex)>;
    using RestoreHandler = std::function<void(ElementIndex)>;

private:

    /*
    * The endpoints of a triangle.
    */
    struct Endpoints
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;
        ElementIndex PointCIndex;

        Endpoints(
            ElementIndex pointAIndex,
            ElementIndex pointBIndex,
            ElementIndex pointCIndex)
            : PointAIndex(pointAIndex)
            , PointBIndex(pointBIndex)
            , PointCIndex(pointCIndex)
        {}
    };

    using SubSpringsVector = FixedSizeVector<ElementIndex, 4u>;

public:

    Triangles(ElementCount elementCount)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, mElementCount, true)
        // Endpoints
        , mEndpointsBuffer(mBufferElementCount, mElementCount, Endpoints(NoneElementIndex, NoneElementIndex, NoneElementIndex))
        // Sub springs
        , mSubSpringsBuffer(mBufferElementCount, mElementCount, SubSpringsVector())
        , mFactorySubSpringsBuffer(mBufferElementCount, mElementCount, SubSpringsVector())
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mDestroyHandler()
        , mRestoreHandler()
    {
    }

    Triangles(Triangles && other) = default;

    /*
     * Sets a (single) handler that is invoked whenever a triangle is destroyed.
     *
     * The handler is invoked right before the triangle is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted triangle might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other triangles from it is not supported
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
     * Sets a (single) handler that is invoked whenever a triangle is restored.
     *
     * The handler is invoked right after the triangle is modified to be restored. However,
     * other elements connected to the soon-to-be-restored triangle might not have been
     * restored yet.
     *
     * The handler is not re-entrant: restoring other triangles from it is not supported
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
        ElementIndex pointCIndex,
        SubSpringsVector const & subSprings);

    void Destroy(ElementIndex triangleElementIndex);

    void Restore(ElementIndex triangleElementIndex);

    //
    // Render
    //

    /*
     * Uploads triangle elements.
     *
     * The planeIndices container contains, for each plane, the starting index of the triangles in that plane into a single
     * buffer for all triangles. The last element contains the total number of (non-deleted) triangles.
     *
     * The content of the planeIndices container is modified by this method, for performance convenience only.
     */
    template<typename TIndices>
    void UploadElements(
        TIndices & planeIndices,
        ShipId shipId,
        Points const & points,
        Render::RenderContext & renderContext) const
    {
        for (ElementIndex i : *this)
        {
            if (!mIsDeletedBuffer[i])
            {
                // Get the plane of this triangle (== plane of point A)
                PlaneId planeId = points.GetPlaneId(GetPointAIndex(i));

                // Send triangle to its index
                assert(planeId < planeIndices.size());
                renderContext.UploadShipElementTriangle(
                    shipId,
                    planeIndices[planeId],
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    GetPointCIndex(i));

                // Remember that the next triangle for this plane goes to the next element
                planeIndices[planeId]++;
            }
        }
    }

public:

    //
    // IsDeleted
    //

    inline bool IsDeleted(ElementIndex triangleElementIndex) const
    {
        return mIsDeletedBuffer[triangleElementIndex];
    }

    //
    // Endpoints
    //

    inline ElementIndex GetPointAIndex(ElementIndex triangleElementIndex) const
    {
        return mEndpointsBuffer[triangleElementIndex].PointAIndex;
    }

    inline ElementIndex GetPointBIndex(ElementIndex triangleElementIndex) const
    {
        return mEndpointsBuffer[triangleElementIndex].PointBIndex;
    }

    inline ElementIndex GetPointCIndex(ElementIndex triangleElementIndex) const
    {
        return mEndpointsBuffer[triangleElementIndex].PointCIndex;
    }

    //
    // Sub springs
    //

    auto const & GetSubSprings(ElementIndex triangleElementIndex) const
    {
        return mSubSpringsBuffer[triangleElementIndex];
    }

    inline void AddSubSpring(
        ElementIndex triangleElementIndex,
        ElementIndex subSpringElementIndex)
    {
        assert(mFactorySubSpringsBuffer[triangleElementIndex].contains(
            [subSpringElementIndex](auto ss)
            {
                return ss = subSpringElementIndex;
            }));

        mSubSpringsBuffer[triangleElementIndex].push_back(subSpringElementIndex);
    }

    inline void RemoveSubSpring(
        ElementIndex triangleElementIndex,
        ElementIndex subSpringElementIndex)
    {
        bool found = mSubSpringsBuffer[triangleElementIndex].erase_first(subSpringElementIndex);

        assert(found);
        (void)found;
    }

    void ClearSubSprings(ElementIndex triangleElementIndex)
    {
        mSubSpringsBuffer[triangleElementIndex].clear();
    }

    auto const & GetFactorySubSprings(ElementIndex triangleElementIndex) const
    {
        return mFactorySubSpringsBuffer[triangleElementIndex];
    }

    void RestoreFactorySubSprings(ElementIndex triangleElementIndex)
    {
        assert(mSubSpringsBuffer[triangleElementIndex].empty());

        for (auto s : mFactorySubSpringsBuffer[triangleElementIndex])
        {
            mSubSpringsBuffer[triangleElementIndex].push_back(s);
        }
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Endpoints
    Buffer<Endpoints> mEndpointsBuffer;

    // Sub springs - the springs that have this triangle among their super-triangles.
    // This is the three springs along the edges, plus the eventual "traverse" spring (i.e. the non-edge diagonal
    // in a two-triangle square)
    Buffer<SubSpringsVector> mSubSpringsBuffer;
    Buffer<SubSpringsVector> mFactorySubSpringsBuffer;

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // The handler registered for triangle deletions
    DestroyHandler mDestroyHandler;

    // The handler registered for triangle restoration
    RestoreHandler mRestoreHandler;
};

}
