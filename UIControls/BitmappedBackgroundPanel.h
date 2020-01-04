/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-01-04
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/custombgwin.h>
#include <wx/wx.h>

class BitmappedBackgroundPanel : public wxCustomBackgroundWindow<wxPanel>
{
public:

    BitmappedBackgroundPanel(
        wxWindow * parent,
        wxWindowID id,
        wxPoint const & position,
        wxSize const & size,
        wxBitmap const & background)
    {
        Create(parent, id, position, size);
        SetBackgroundBitmap(background);
    }
};
