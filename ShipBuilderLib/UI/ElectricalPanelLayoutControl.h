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
#include <wx/scrolwin.h>

#include <functional>
#include <map>
#include <optional>

namespace ShipBuilder {

class ElectricalPanelLayoutControl : public wxScrolled<wxPanel>
{
public:

    ElectricalPanelLayoutControl(
        wxWindow * parent,
        std::function<void(ElectricalElementInstanceIndex instanceIndex)> onElementSelected,
        ResourceLocator const & resourceLocator);

    void SetPanel(ElectricalPanelMetadata const & electricalPanelMetadata);

    void SelectElement(ElectricalElementInstanceIndex instanceIndex);
    void SetElementVisible(ElectricalElementInstanceIndex instanceIndex, ElectricalPanelElementMetadata const & panelMetadata, bool isVisible);

private:

    void OnCloseWindow(wxCloseEvent & event);
    void OnLeftMouseDown(wxMouseEvent & event);
    void OnLeftMouseUp(wxMouseEvent & event);
    void OnMouseMove(wxMouseEvent & event);
    void OnResized(wxSizeEvent & event);

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

    void RenderSlot(
        wxPen const & pen,
        wxRect && rect,
        wxDC & dc);

    void RenderElement(
        wxRect const & rect,
        wxDC & dc);

    void RecalculateGeometry();

    static wxRect MakeDcRect(
        IntegralCoordinates const & layoutCoordinates,
        int virtualAreaWidth);

private:

    wxPen mFreeUnselectedSlotBorderPen;
    wxPen mOccupiedUnselectedSlotBorderPen;
    wxPen mOccupiedSelectedSlotBorderPen;
    wxPen mDropSlotBorderPen;

    wxBrush mTransparentBrush;

    // TODOHERE
    wxBrush mWaterBrush;
    wxImage mShipImage;

private:

    bool mIsMouseCaptured;

    std::function<void(ElectricalElementInstanceIndex instanceIndex)> const mOnElementSelected;

private:

    int mLayoutCols;
    int mLayoutRows;

    struct Element
    {
        IntegralCoordinates LayoutCoordinates;
        std::optional<wxRect> DcRect;

        Element(IntegralCoordinates layoutCoordinates)
            : LayoutCoordinates(layoutCoordinates)
            , DcRect() // Populated at RecalculateGeometry()
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

    // Calculated
    int mNElementsOnEitherSide; // Not including center element
    int mVirtualAreaWidth;
};

}