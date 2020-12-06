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
#include <array>
#include <cassert>
#include <functional>

namespace Physics
{

class Triangles : public ElementContainer
{
private:

    /*
     * The endpoints of a triangle, in CW order.
     */
    struct Endpoints
    {
        // A, B, C
        std::array<ElementIndex, 3u> PointIndices;

        Endpoints(
            ElementIndex pointAIndex,
            ElementIndex pointBIndex,
            ElementIndex pointCIndex)
            : PointIndices({ pointAIndex, pointBIndex, pointCIndex })
        {}
    };

    /*
     * The springs along the edges of a triangle, in CW order.
     */
    struct SubSprings
    {
        // A, B, C
        std::array<ElementIndex, 3u> SpringIndices;

        SubSprings(
            ElementIndex subSpringAIndex,
            ElementIndex subSpringBIndex,
            ElementIndex subSpringCIndex)
            : SpringIndices({ subSpringAIndex, subSpringBIndex, subSpringCIndex })
        {}
    };

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
        , mSubSpringsBuffer(mBufferElementCount, mElementCount, SubSprings(NoneElementIndex, NoneElementIndex, NoneElementIndex))
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mShipPhysicsHandler(nullptr)
    {
    }

    Triangles(Triangles && other) = default;

    void RegisterShipPhysicsHandler(IShipPhysicsHandler * shipPhysicsHandler)
    {
        mShipPhysicsHandler = shipPhysicsHandler;
    }

    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        ElementIndex pointCIndex,
        ElementIndex subSpringAIndex,
        ElementIndex subSpringBIndex,
        ElementIndex subSpringCIndex);

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

    inline auto const & GetPointIndices(ElementIndex triangleElementIndex) const
    {
        return mEndpointsBuffer[triangleElementIndex].PointIndices;
    }

    inline ElementIndex GetPointAIndex(ElementIndex triangleElementIndex) const
    {
        return mEndpointsBuffer[triangleElementIndex].PointIndices[0];
    }

    inline ElementIndex GetPointBIndex(ElementIndex triangleElementIndex) const
    {
        return mEndpointsBuffer[triangleElementIndex].PointIndices[1];
    }

    inline ElementIndex GetPointCIndex(ElementIndex triangleElementIndex) const
    {
        return mEndpointsBuffer[triangleElementIndex].PointIndices[2];
    }

    inline bool ArePointsInCwOrder(
        ElementIndex triangleElementIndex,
        ElementIndex point1Index,
        ElementIndex point2Index) const
    {
        return (GetPointAIndex(triangleElementIndex) == point1Index && GetPointBIndex(triangleElementIndex) == point2Index)
            || (GetPointBIndex(triangleElementIndex) == point1Index && GetPointCIndex(triangleElementIndex) == point2Index)
            || (GetPointCIndex(triangleElementIndex) == point1Index && GetPointAIndex(triangleElementIndex) == point2Index);
    }

    //
    // Sub springs
    //

    auto const & GetSubSprings(ElementIndex triangleElementIndex) const
    {
        return mSubSpringsBuffer[triangleElementIndex];
    }

    inline ElementIndex GetSubSpringAIndex(ElementIndex triangleElementIndex) const
    {
        return mSubSpringsBuffer[triangleElementIndex].SpringIndices[0];
    }

    inline ElementIndex GetSubSpringBIndex(ElementIndex triangleElementIndex) const
    {
        return mSubSpringsBuffer[triangleElementIndex].SpringIndices[1];
    }

    inline ElementIndex GetSubSpringCIndex(ElementIndex triangleElementIndex) const
    {
        return mSubSpringsBuffer[triangleElementIndex].SpringIndices[2];
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
    Buffer<SubSprings> mSubSpringsBuffer;

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    IShipPhysicsHandler * mShipPhysicsHandler;
};

}
