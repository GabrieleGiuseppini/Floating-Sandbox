/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "ShipBuilderTypes.h"

#include <Simulation/Layers.h>

#include <optional>

namespace ShipBuilder {

/*
 * Just a glorified optional<LayersRegion>, maintaining current clipboard and notifying of state changes.
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

    std::optional<ShipLayers> const & GetContent() const
    {
        return mClipboard;
    }

    void SetContent(std::optional<ShipLayers> && content)
    {
        mClipboard = std::move(content);
        mUserInterface.OnClipboardChanged(mClipboard.has_value());
    }

protected:

    std::optional<ShipLayers> mClipboard;

    IUserInterface & mUserInterface;
};

}