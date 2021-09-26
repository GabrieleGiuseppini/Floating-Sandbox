/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/filedlg.h>

#include <string>

namespace ShipBuilder {

class SaveShipDialog : public wxFileDialog
{
public:

	enum class SaveGoalType
	{
		FullShip,
		StructuralLayer
	};

public:

	SaveShipDialog(
		wxWindow * parent,
		std::string const & shipName,
		SaveGoalType goal);
};

}