/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-03-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/filedlg.h>

#include <filesystem>

class ImageSaveDialog : public wxFileDialog
{
public:

	ImageSaveDialog(wxWindow * parent);

	std::filesystem::path GetChosenFilepath() const
	{
		return std::filesystem::path(GetPath().ToStdString());
	}
};
