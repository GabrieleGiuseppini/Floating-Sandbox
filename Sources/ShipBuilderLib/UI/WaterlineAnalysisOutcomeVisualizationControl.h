/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-08
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <wx/dc.h>
#include <wx/image.h>
#include <wx/panel.h>

#include <optional>

namespace ShipBuilder {

class WaterlineAnalysisOutcomeVisualizationControl : public wxPanel
{
public:

    WaterlineAnalysisOutcomeVisualizationControl(
        wxWindow * parent,
        GameAssetManager const & gameAssetManager);

    void SetValue(
        float trimCW, // radians, 0 vertical
        bool floats);

    void Clear();

private:

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

private:

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