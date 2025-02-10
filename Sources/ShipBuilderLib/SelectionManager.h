/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "ShipBuilderTypes.h"

#include <optional>

namespace ShipBuilder {

/*
 * Just a glorified optional<Rect>, maintaining current selection and notifying of state changes.
 */
class SelectionManager final
{
public:

    SelectionManager(IUserInterface & userInterface)
        : mUserInterface(userInterface)
    {}

    std::optional<ShipSpaceRect> const & GetSelection() const
    {
        return mSelection;
    }

    void SetSelection(std::optional<ShipSpaceRect> selection)
    {
        mSelection = selection;
        mUserInterface.OnSelectionChanged(mSelection);
    }

protected:

    std::optional<ShipSpaceRect> mSelection;

    IUserInterface & mUserInterface;
};

}