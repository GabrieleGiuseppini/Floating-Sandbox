/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"

#include <cassert>
#include <functional>
#include <memory>

namespace Physics
{

class ElectricalElements : public ElementContainer
{
public:

    using DestroyHandler = std::function<void(ElementIndex)>;

public:

    ElectricalElements(ElementCount elementCount)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(elementCount)
        , mElectricalElementBuffer(elementCount)        
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mDestroyHandler()
    {
    }

    ElectricalElements(ElectricalElements && other) = default;

    /*
     * Sets a (single) handler that is invoked whenever an electrical element is destroyed.
     *
     * The handler is invoked right before the electrical element is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted electrical element might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other electrical elements from it is not supported 
     * and leads to undefined behavior.
     *
     * Setting more than one handler is not supported and leads to undefined behavior.
     */
    void RegisterDestroyHandler(DestroyHandler destroyHandler)
    {
        assert(!mDestroyHandler);
        mDestroyHandler = std::move(destroyHandler);
    }

    void Add(std::unique_ptr<ElectricalElement> electricalElement);

    void Destroy(ElementIndex electricalElementIndex);

public:

    inline bool IsDeleted(ElementIndex electricalElementIndex) const
    {
        assert(electricalElementIndex < mElementCount);

        return mIsDeletedBuffer[electricalElementIndex];
    }

    inline ElectricalElement * GetElectricalElement(ElementIndex electricalElementIndex) const
    {
        assert(electricalElementIndex < mElementCount);

        return mElectricalElementBuffer[electricalElementIndex].get();
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Endpoints
    Buffer<std::unique_ptr<ElectricalElement>> mElectricalElementBuffer;

    //////////////////////////////////////////////////////////
    // Container 
    //////////////////////////////////////////////////////////

    // The handler registered for electrical element deletions
    DestroyHandler mDestroyHandler;
};

}
