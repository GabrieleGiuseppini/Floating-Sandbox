/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipSaveDialog.h"

#include <UILib/StandardSystemPaths.h>

#include <Game/ShipDeSerializer.h>

namespace ShipBuilder {

ShipSaveDialog::ShipSaveDialog(wxWindow * parent)
	: wxFileDialog(
		parent,
		wxEmptyString,
		StandardSystemPaths::GetInstance().GetUserShipFolderPath().string() 
			+ std::filesystem::path::preferred_separator, // See https://forums.wxwidgets.org/viewtopic.php?t=42293
		wxEmptyString,
		wxEmptyString,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
		wxDefaultPosition,
		wxDefaultSize)
{
}

int ShipSaveDialog::ShowModal(
	std::string const & shipFilename,
	GoalType goal)
{
	std::string shipFileExtension;

	switch (goal)
	{
		case GoalType::FullShip:
		{
			SetMessage(_("Save this ship"));

			shipFileExtension = ShipDeSerializer::GetShipDefinitionFileExtension();
			SetWildcard(_("Ship files") + wxS(" (*") + shipFileExtension + wxS(")|*") + shipFileExtension);

			break;
		}

		case GoalType::StructuralLayer:
		{
			SetMessage(_("Save the structural layer"));

			shipFileExtension = ShipDeSerializer::GetImageDefinitionFileExtension();
			SetWildcard(_("Structure-only image files") + wxS(" (*") + shipFileExtension + + wxS(")|*") + shipFileExtension);

			break;
		}
	}

	SetFilename(shipFilename + shipFileExtension);

	return ShowModal();
}

}