                    /***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/panel.h>

class UnFocusablePanel : public wxPanel
{
public:

    UnFocusablePanel()
        :wxPanel()
    {}

    UnFocusablePanel(wxWindow * parent, long style = 0)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
    {}

    virtual bool AcceptsFocus() const final override
    {
        return false;
    }
};