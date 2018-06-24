/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void ElectricalElements::Add(std::unique_ptr<ElectricalElement> electricalElement)
{
    mIsDeletedBuffer.emplace_back(false);

    mElectricalElementBuffer.emplace_back(std::move(electricalElement));
}

void ElectricalElements::Destroy(ElementIndex electricalElementIndex)
{
    assert(electricalElementIndex < mElementCount);
    assert(!IsDeleted(electricalElementIndex));

    // Invoke destroy handler
    if (!!mDestroyHandler)
    {
        mDestroyHandler(electricalElementIndex);
    }

    // Flag ourselves as deleted
    mIsDeletedBuffer[electricalElementIndex] = true;
}

}