/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/statbmp.h>
#include <wx/statusbr.h>

#include <optional>

namespace ShipBuilder {

class StatusBar : public wxStatusBar
{
public:

    StatusBar(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    void SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates);

private:

    void OnResize(wxSizeEvent & event);

private:

    // TODO: static bitmaps
};

}