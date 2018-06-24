/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

Generator::Generator(ElementIndex pointIndex)
    : ElectricalElement(        
        pointIndex,
        ElectricalElement::Type::Generator)
{
}

}