/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <optional>

namespace ShipBuilder {

/*
 * Interface of MainFrame that is seen by Controller and underneath.
 */
struct IUserInterface
{
public:

    // Notifies of a change in the size of the model
    virtual void OnWorkSpaceSizeChanged() = 0;

    // Notifies of a change in any member of the workbench state
    virtual void OnWorkbenchStateChanged() = 0;

    virtual void DisplayToolCoordinates(std::optional<WorkSpaceCoordinates> coordinates) = 0;
};

}