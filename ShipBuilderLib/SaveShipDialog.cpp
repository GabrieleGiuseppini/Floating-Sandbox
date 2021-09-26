/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SaveShipDialog.h"

#include <UILib/StandardSystemPaths.h>

namespace ShipBuilder {

SaveShipDialog::SaveShipDialog(
	wxWindow * parent,
	std::string const & shipName,
	SaveGoalType goal)
	: wxFileDialog(
		parent,
		goal == SaveGoalType::FullShip ? _("Save this ship") : _("Save the structural layer"),
		StandardSystemPaths::GetInstance().GetUserShipFolderPath().string(),
		shipName, // TODOTEST
		goal == SaveGoalType::FullShip ? "SHP2 files (*.shp2)|*.shp2" : "PNG files(*.png) | *.png", // TODOTEST
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
		wxDefaultPosition,
		wxDefaultSize) // TODOTEST: wxSize(600, 600),
{
}

}