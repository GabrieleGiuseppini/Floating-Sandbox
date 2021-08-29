/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "WorkbenchState.h"

#include <Game/MaterialDatabase.h>

namespace ShipBuilder {

class MaterialPalette
{
public:

    MaterialPalette(
        MaterialDatabase const & materialDatabase,
        WorkbenchState & workbenchState);

private:

    WorkbenchState & mWorkbenchState;
};

}