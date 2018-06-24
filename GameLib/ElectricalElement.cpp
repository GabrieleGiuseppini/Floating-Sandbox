/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

ElectricalElement::ElectricalElement(    
    ElementIndex pointIndex,
    Type type)
    : mPointIndex(pointIndex)
    , mType(type)
    , mLastGraphVisitStepSequenceNumber(0u)
{
}
}