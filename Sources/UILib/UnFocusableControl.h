/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-10-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/wx.h>

template<typename TControl>
class UnFocusableControl : public TControl
{
public:

    UnFocusableControl()
        :TControl()
    {}

    UnFocusableControl(wxWindow * parent, wxSize const & size = wxDefaultSize, long style = 0)
        : TControl(parent, wxID_ANY, wxDefaultPosition, size, style)
    {}

    virtual bool AcceptsFocus() const final override
    {
        return false;
    }
};