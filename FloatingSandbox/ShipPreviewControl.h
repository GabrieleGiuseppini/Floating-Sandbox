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

    ShipPreviewControl(wxWindow * parent);

    virtual ~ShipPreviewControl();

private:

    void OnMouseClick(wxMouseEvent & event);
};
