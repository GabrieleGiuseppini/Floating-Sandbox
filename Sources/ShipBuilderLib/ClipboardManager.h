/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "ShipBodyRegions.h"

#include <optional>

namespace ShipBuilder {

/*
 * Just a glorified optional<ShipSpaceBodyRegions::ShipSpaceBodyRegion>, maintaining current clipboard and notifying of state changes.
 */
class ClipboardManager final
{
public:

    ClipboardManager(IUserInterface & userInterface)
        : mUserInterface(userInterface)
    {}

    bool IsEmpty() const
    {
        return !mClipboard.has_value();
    }

    std::optional<ShipSpaceBodyRegions::ShipSpaceBodyRegion> const & GetContent() const
    {
        return mClipboard;
    }

    void SetContent(std::optional<ShipSpaceBodyRegions::ShipSpaceBodyRegion> && content)
    {
        mClipboard = std::move(content);
        mUserInterface.OnClipboardChanged(mClipboard.has_value());
    }

protected:

    std::optional<ShipSpaceBodyRegions::ShipSpaceBodyRegion> mClipboard;

    IUserInterface & mUserInterface;
};

}