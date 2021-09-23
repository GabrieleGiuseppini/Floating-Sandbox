/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <wx/statusbr.h>

#include <optional>

namespace ShipBuilder {

class StatusBar : public wxStatusBar
{
public:

    StatusBar(wxWindow * parent);

    void SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates);

private:

};

}