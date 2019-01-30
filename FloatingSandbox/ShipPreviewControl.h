/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/wx.h>

class ShipPreviewControl : public wxPanel
{
public:

    ShipPreviewControl(
        wxWindow * parent,
        int width,
        int height);

    virtual ~ShipPreviewControl();

private:

    void OnMouseClick(wxMouseEvent & event);

private:

    int const mWidth;
    int const mHeight;
};
