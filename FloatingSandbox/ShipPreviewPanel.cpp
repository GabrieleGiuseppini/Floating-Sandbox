/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewPanel.h"

ShipPreviewPanel::ShipPreviewPanel(wxWindow* parent)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

ShipPreviewPanel::~ShipPreviewPanel()
{
}