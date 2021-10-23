/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipSaveDialog.h"

#include <UILib/StandardSystemPaths.h>

#include <Game/ShipDeSerializer.h>

#include <GameCore/Utils.h>

namespace ShipBuilder {

ShipSaveDialog::ShipSaveDialog(wxWindow * parent)
	: wxFileDialog(
		parent,
		wxEmptyString,
		StandardSystemPaths::GetInstance().GetUserShipFolderPath().string(),
		wxEmptyString,
		wxEmptyString,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
		wxDefaultPosition,
		wxDefaultSize)
{
}

int ShipSaveDialog::ShowModal(
	std::string const & shipName,
	GoalType goal)
{
	SetFilename(Utils::MakeFilenameSafeString(shipName));

	switch (goal)
	{
		case GoalType::FullShip:
		{
			SetMessage(_("Save this ship"));

			auto const ext = ShipDeSerializer::GetShipDefinitionFileExtension();
			SetWildcard(_("Ship files") + wxS(" (*") + ext + wxS(")|*") + ext);

			break;
		}

		case GoalType::StructuralLayer:
		{
			SetMessage(_("Save the structural layer"));

			auto const ext = ShipDeSerializer::GetImageDefinitionFileExtension();
			SetWildcard(_("Structure-only image files") + wxS(" (*") + ext + + wxS(")|*") + ext);

			break;
		}
	}

	return ShowModal();
}

}