/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"
#include "FixedSizeVector.h"
#include "GameParameters.h"
#include "Material.h"
#include "RenderContext.h"

#include <cassert>
#include <functional>

namespace Physics
{

class Triangles : public ElementContainer
{
public:

    using DestroyHandler = std::function<void(ElementIndex)>;

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
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mDestroyHandler()
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

    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        ElementIndex pointCIndex,
        SubSpringsVector const & subSprings);

    void Destroy(ElementIndex triangleElementIndex);

    //
    // Render
    //

    void UploadElements(
        ShipId shipId,
        Render::RenderContext & renderContext,
        Points const & points) const;

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

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // The handler registered for triangle deletions
    DestroyHandler mDestroyHandler;
};

}
