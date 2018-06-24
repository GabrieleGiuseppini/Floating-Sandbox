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


public:

    Triangles(ElementCount elementCount)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(elementCount)
        // Endpoints
        , mEndpointsBuffer(elementCount)        
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
        ElementIndex pointCIndex);

    void Destroy(ElementIndex triangleElementIndex);

    void UploadElements(
        int shipId,
        RenderContext & renderContext,
        Points const & points) const;

public:

    //
    // IsDeleted
    //

    inline bool IsDeleted(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mIsDeletedBuffer[triangleElementIndex];
    }

    //
    // Endpoints
    //

    inline ElementIndex GetPointAIndex(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mEndpointsBuffer[triangleElementIndex].PointAIndex;
    }

    inline ElementIndex GetPointBIndex(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mEndpointsBuffer[triangleElementIndex].PointBIndex;
    }

    inline ElementIndex GetPointCIndex(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mEndpointsBuffer[triangleElementIndex].PointCIndex;
    }

private:

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Endpoints
    Buffer<Endpoints> mEndpointsBuffer;

    //////////////////////////////////////////////////////////
    // Container 
    //////////////////////////////////////////////////////////

    // The handler registered for triangle deletions
    DestroyHandler mDestroyHandler;
};

}
