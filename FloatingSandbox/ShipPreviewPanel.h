/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipPreviewControl.h"

#include <wx/sizer.h>
#include <wx/wx.h>

#include <filesystem>
#include <memory>
#include <string>

class ShipPreviewPanel : public wxPanel
{
public:

    ShipPreviewPanel(
        wxWindow* parent);

	virtual ~ShipPreviewPanel();

    void SetDirectory(std::filesystem::path const & directoryPath);

private:

    wxGridSizer * mPreviewsSizer;
};
