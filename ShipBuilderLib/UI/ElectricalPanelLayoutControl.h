/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/dc.h>
#include <wx/image.h>
#include <wx/panel.h>

#include <optional>

namespace ShipBuilder {

class ElectricalPanelLayoutControl : public wxPanel
{
public:

    ElectricalPanelLayoutControl(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    // TODOHERE
    ////void SetValue(
    ////    float trimCW, // radians, 0 vertical
    ////    bool floats);

    void Clear();

private:

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

private:

    // TODOHERE
    wxPen mGuidePen;
    wxPen mWaterlinePen;
    wxPen mWaterPen;
    wxBrush mWaterBrush;
    wxImage mShipImage;

    // State

    struct Outcome
    {
        float TrimCW;
        bool Floats;

        Outcome(
            float trimCW,
            bool floats)
            : TrimCW(trimCW)
            , Floats(floats)
        {}
    };

    std::optional<Outcome> mOutcome;
};

}