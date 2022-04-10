/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "InstancedElectricalElementSet.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/bitmap.h>
#include <wx/dc.h>
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

    void SetPanel(
        InstancedElectricalElementSet const & instancedElectricalElementSet,
        ElectricalPanelMetadata const & electricalPanelMetadata);

    void SelectElement(ElectricalElementInstanceIndex instanceIndex);
    void SetElementVisible(ElectricalElementInstanceIndex instanceIndex, ElectricalPanelElementMetadata const & panelMetadata, bool isVisible);

private:

    struct Element;

    void OnCloseWindow(wxCloseEvent & event);
    void OnLeftMouseDown(wxMouseEvent & event);
    void OnLeftMouseUp(wxMouseEvent & event);
    void OnMouseMove(wxMouseEvent & event);
    void OnResized(wxSizeEvent & event);

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

    void RenderSlot(
        wxRect const & virtualRect,
        int virtualOriginX,
        wxPen const & pen,
        wxBrush const & brush,
        wxDC & dc);

    void RenderElement(
        ElectricalElementInstanceIndex instanceIndex,
        wxRect const & virtualRect,
        int virtualOriginX,
        bool isBeingMoved,
        wxDC & dc);

    void RecalculateGeometry();

    int GetOriginVirtualX() const;

    wxPoint ClientToVirtual(wxPoint const & clientCoords) const;

    wxRect ClientToVirtual(wxRect const & clientCoords) const;

    wxRect MakeSlotVirtualRect(IntegralCoordinates const & layoutCoordinates) const;

    // TODOOLD



    std::optional<IntegralCoordinates> GetSlotCoordinatesAt(wxPoint const & virtualCoords) const;

    std::optional<std::tuple<ElectricalElementInstanceIndex, Element const &>> GetExistingElementAt(wxPoint const & virtualCoords) const;

    std::optional<std::tuple<ElectricalElementInstanceIndex, Element const &>> GetExistingElementAt(IntegralCoordinates const & layoutCoordinates) const;


private:

    wxPen mGuidePen;
    wxPen mFreeUnselectedSlotBorderPen;
    wxPen mOccupiedUnselectedSlotBorderPen;
    wxPen mOccupiedSelectedSlotBorderPen;
    wxBrush mOccupiedSelectedSlotBorderBrush;
    wxPen mDropSlotBorderPen;
    wxBrush mDropSlotBorderBrush;
    wxPen mShadowPen;
    wxBrush mShadowBrush;
    wxPen mTransparentPen;
    wxBrush mTransparentBrush;
    wxFont mInstanceIndexFont;

    wxBitmap mElementBitmap;
    int const mElementWidth;
    int const mElementHeight;
    int const mPanelHeight;

private:

    bool mIsMouseCaptured;

    std::function<void(ElectricalElementInstanceIndex instanceIndex)> const mOnElementSelected;

private:

    int mLayoutCols;
    int mLayoutRows;

    struct Element
    {
        IntegralCoordinates LayoutCoordinates;
        std::optional<wxRect> DcRect; // Virtual coords

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

    std::optional<IntegralCoordinates> mCurrentDropCandidateSlotCoordinates;

    // Calculated
    int mNElementsOnEitherSide; // Not including center element
    int mVirtualAreaWidth;
};

}