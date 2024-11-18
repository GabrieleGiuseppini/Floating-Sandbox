/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../InstancedElectricalElementSet.h"

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

    bool IsDirty() const
    {
        assert(mSessionData.has_value());
        return mSessionData->IsDirty;
    }

    // Invoked to populate and start a new usage session
    void SetPanel(ElectricalPanel & electricalPanel);

    // Invoked to un-populate and stop the current usage session
    void ResetPanel();

    void SelectElement(ElectricalElementInstanceIndex instanceIndex);

    void OnPanelUpdated();

private:

    struct LayoutSlot;

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

    void ScrollToCenter();

    void RecalculateLayout();

    wxPoint ClientToVirtual(wxPoint const & clientCoords) const;

    wxRect ClientToVirtual(wxRect const & clientCoords) const;

    wxRect MakeSlotVirtualRect(IntegralCoordinates const & layoutCoordinates) const;

    std::optional<IntegralCoordinates> GetSlotCoordinatesAt(wxPoint const & virtualCoords) const;

    IntegralCoordinates const & GetLayoutCoordinatesOf(ElectricalElementInstanceIndex instanceIndex) const;

private:

    wxPen mGuidePen;
    wxPen mFreeUnselectedSlotBorderPen;
    wxPen mOccupiedUnselectedSlotBorderPen;
    wxPen mOccupiedSelectedSlotBorderPen;
    wxBrush mOccupiedSelectedSlotBorderBrush;
    wxPen mDropSlotBorderPen;
    wxBrush mDropSlotBorderBrush;
    wxPen mTransparentPen;
    wxBrush mTransparentBrush;
    wxFont mInstanceIndexFont;

    wxBitmap mElementBitmap;
    wxBitmap mElementShadowBitmap;
    int const mElementWidth;
    int const mElementHeight;
    int const mPanelHeight;

private:

    std::function<void(ElectricalElementInstanceIndex instanceIndex)> const mOnElementSelected;

    //
    // State
    //

    struct SessionData
    {
        ElectricalPanel & Panel;
        bool IsDirty;

        SessionData(ElectricalPanel & electricalPanel)
            : Panel(electricalPanel)
            , IsDirty(false)
        {}
    };

    std::optional<SessionData> mSessionData;

    bool mIsMouseCaptured;

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

    //
    // Layout
    //

    int mVirtualAreaWidth;
    int mNElementsOnEitherSide; // Not including center element

    struct LayoutSlot
    {
        // When set, there's an element here
        std::optional<ElectricalElementInstanceIndex> OccupyingInstanceIndex;

        // Virtual rect of this slot
        wxRect SlotVirtualRect;

        LayoutSlot(
            std::optional<ElectricalElementInstanceIndex> occupyingInstanceIndex,
            wxRect const & slotVirtualRect)
            : OccupyingInstanceIndex(occupyingInstanceIndex)
            , SlotVirtualRect(slotVirtualRect)
        {}
    };

    std::map<IntegralCoordinates, LayoutSlot> mLayoutSlotsByLayoutCoordinates;
};

}