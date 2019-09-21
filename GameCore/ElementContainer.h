/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ElementIndexRangeIterator.h"
#include "GameTypes.h"
#include "SysSpecifics.h"

#include <cassert>
#include <cstdint>

/*
 * This class is the base class of all containers of core elements, providing
 * services that are common to all containers.
 *
 * For data locality, we don't work with "objects" in the OO way, but rather
 * with sets of objects, whose properties are located in multiple, non-overlapping buffers.
 *
 * The container itself is not modifiable once all its elements have been created.
 */
class ElementContainer
{
public:

    /*
     * Gets the number of elements in this container.
     */
    ElementCount GetElementCount() const { return mElementCount; }

    /*
     * Gets the  number of elements in the buffers of this container.
     * The number of elements in the buffers is a multiple of the vectorized
     * word size.
     */
    ElementCount GetBufferElementCount() const { return mBufferElementCount; }

    /*
     * Visitors. These iterators iterate the (non-vectorized) *indices* of the elements.
     */

    inline element_index_range_iterator begin() const noexcept
    {
        return element_index_range_iterator(0u);
    }

    inline element_index_range_iterator end() const noexcept
    {
        return element_index_range_iterator(mElementCount);
    }

    inline ElementIndexRangeIterable BufferElements() const noexcept
    {
        return ElementIndexRangeIterable(0u, mBufferElementCount);
    }

protected:

    ElementContainer(ElementCount elementCount)
        : mElementCount(elementCount)
        // We round our number of buffer elements to the next multiple of the vectorized float count, so that
        // buffers of single floats are aligned on vectorized word boundaries.
        // Note that buffers of more than single floats would also automatically be aligned.
        , mBufferElementCount(make_aligned_float_element_count(elementCount))
    {
    }

    // The actual number of elements in this container
    ElementCount const mElementCount;

    // The number of elements available in the *buffers* of this container;
    // differs from the element count as this is rounded up to the
    // vectorization word size
    ElementCount const mBufferElementCount;
};
