/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/dc.h>
#include <wx/image.h>
#include <wx/panel.h>

#include <functional>
#include <map>
#include <optional>

namespace ShipBuilder {

class ElectricalPanelLayoutControl : public wxPanel
{
public:

    ElectricalPanelLayoutControl(
        wxWindow * parent,
        std::function<void(ElectricalElementInstanceIndex instanceIndex)> onElementSelected,
        ResourceLocator const & resourceLocator);

    void SetPanel(ElectricalPanelMetadata const & electricalPanelMetadata);

    void SelectElement(ElectricalElementInstanceIndex instanceIndex);

private:

    void OnLeftMouseDown(wxMouseEvent & event);
    void OnLeftMouseUp(wxMouseEvent & event);
    void OnMouseMove(wxMouseEvent & event);

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

    void RenderElement(
        wxRect const & rect,
        wxDC & dc);

    wxRect MakeDcRect(IntegralCoordinates const & layoutCoordinates);

private:

    // TODOHERE
    wxPen mGuidePen;
    wxPen mWaterlinePen;
    wxPen mWaterPen;
    wxBrush mWaterBrush;
    wxImage mShipImage;

private:

    std::function<void(ElectricalElementInstanceIndex instanceIndex)> const mOnElementSelected;

    struct Element
    {
        IntegralCoordinates LayoutCoordinates;
        wxRect DcRect;

        Element(
            IntegralCoordinates layoutCoordinates,
            wxRect const & dcRect)
            : LayoutCoordinates(layoutCoordinates)
            , DcRect(dcRect)
        {}
    };

    std::map<ElectricalElementInstanceIndex, Element> mElements;

    struct MovableElement
    {
        ElectricalElementInstanceIndex const InstanceIndex;
        wxPoint const InRectAnchorMouseCoords;
        wxPoint CurrentMouseCoords;

        MovableElement(
            ElectricalElementInstanceIndex instanceIndex,
            wxPoint const & inRectAnchorMouseCoords,
            wxPoint const & currentMouseCoords)
            : InstanceIndex(instanceIndex)
            , InRectAnchorMouseCoords(inRectAnchorMouseCoords)
            , CurrentMouseCoords(currentMouseCoords)
        {}
    };

    std::optional<MovableElement> mCurrentlyMovableElement; // When set, L mouse is down

    std::optional<ElectricalElementInstanceIndex> mCurrentlySelectedElementInstanceIndex;

    wxSize mVirtualPanelSize;
};

}