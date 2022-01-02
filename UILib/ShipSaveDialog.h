/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/filedlg.h>

#include <filesystem>
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
		std::string const & shipFilename,
		GoalType goal);

	std::filesystem::path GetChosenShipFilepath() const
	{
		return std::filesystem::path(GetPath().ToStdString());
	}

private:

	using wxFileDialog::ShowModal;
};

}