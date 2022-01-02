/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/panel.h>
#include <wx/scrolwin.h>

class UnFocusableScrollablePanel : public wxScrolled<wxPanel>
{
public:

    UnFocusableScrollablePanel(wxWindow * parent, long style = 0)
        : wxScrolled<wxPanel>(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
    {}

    virtual bool AcceptsFocus() const final override
    {
        return false;
    }
};