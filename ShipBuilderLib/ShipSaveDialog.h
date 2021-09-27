/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/filedlg.h>

#include <string>

namespace ShipBuilder {

class ShipSaveDialog : public wxFileDialog
{
public:

	enum class GoalType
	{
		FullShip,
		StructuralLayer
	};

public:

	ShipSaveDialog(wxWindow * parent);

	int ShowModal(
		std::string const & shipName,
		GoalType goal);

private:

	using wxFileDialog::ShowModal;
};

}