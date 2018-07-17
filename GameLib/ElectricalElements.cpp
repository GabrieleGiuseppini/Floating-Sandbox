/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void ElectricalElements::Add(
    ElementIndex pointElementIndex,
    Material::ElectricalProperties::ElectricalElementType elementType,
    bool isSelfPowered)
{
    mIsDeletedBuffer.emplace_back(false);
    mPointIndexBuffer.emplace_back(pointElementIndex);
    mTypeBuffer.emplace_back(elementType);
    mConnectedElectricalElementsBuffer.emplace_back();
    mAvailableCurrentBuffer.emplace_back(0.f);

    switch (elementType)
    {
        case Material::ElectricalProperties::ElectricalElementType::Cable:
        {
            mElementStateBuffer.emplace_back(ElementState::CableState());
            break;
        }

        case Material::ElectricalProperties::ElectricalElementType::Generator:
        {
            mGenerators.emplace_back(static_cast<ElementIndex>(mElementStateBuffer.GetCurrentSize()));
            mElementStateBuffer.emplace_back(ElementState::GeneratorState());
            break;
        }

        case Material::ElectricalProperties::ElectricalElementType::Lamp:
        {
            mLamps.emplace_back(static_cast<ElementIndex>(mElementStateBuffer.GetCurrentSize()));
            mElementStateBuffer.emplace_back(ElementState::LampState(isSelfPowered));
            break;
        }
    }
    
    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back(NoneVisitSequenceNumber);
}

void ElectricalElements::Destroy(ElementIndex electricalElementIndex)
{
    assert(electricalElementIndex < mElementCount);
    assert(!IsDeleted(electricalElementIndex));

    // Zero out our current
    mAvailableCurrentBuffer[electricalElementIndex] = 0.0f;

    // Note: no need to remove self from connected electrical elements, as Ship's PointDestroyHandler,
    // which is the caller of this Destroy(), has already destroyed the point's springs, hence
    // this electrical element has no connected points anymore already and viceversa
    assert(GetConnectedElectricalElements(electricalElementIndex).empty());

    // Invoke destroy handler
    if (!!mDestroyHandler)
    {
        mDestroyHandler(electricalElementIndex);
    }

    // Flag ourselves as deleted
    mIsDeletedBuffer[electricalElementIndex] = true;
}

void ElectricalElements::Update(
    VisitSequenceNumber currentConnectivityVisitSequenceNumber,
    GameParameters const & gameParameters)
{
    //
    // Visit all lamps and run their state machine
    //

    for (auto iLamp : mLamps)
    {
        if (!mIsDeletedBuffer[iLamp])
        {
            RunLampStateMachine(
                iLamp,
                currentConnectivityVisitSequenceNumber,
                gameParameters);
        }
        else
        {
            assert(0.0f == mAvailableCurrentBuffer[iLamp]);
        }
    }
}

void ElectricalElements::RunLampStateMachine(
    ElementIndex elementLampIndex,
    VisitSequenceNumber currentConnectivityVisitSequenceNumber,
    GameParameters const & gameParameters)
{
    if (mElementStateBuffer[elementLampIndex].Lamp.IsSelfPowered)
    {
        //
        // Self-powered lamp, always on
        //

        mAvailableCurrentBuffer[elementLampIndex] = 1.0f;
    }
    else
    {
        //
        // Normal lamp, only on if visited, and controlled by flicker state machine
        //

        // TODOTEST: flicker state machine is missing
        mAvailableCurrentBuffer[elementLampIndex] =
            (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]) ?
            1.0f : 0.0f;
    }
}

}