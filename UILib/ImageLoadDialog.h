/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/filedlg.h>

#include <filesystem>

class ImageLoadDialog : public wxFileDialog
{
public:

	ImageLoadDialog(wxWindow * parent);

	std::filesystem::path GetChosenFilepath() const
	{
		return std::filesystem::path(GetPath().ToStdString());
	}
};
