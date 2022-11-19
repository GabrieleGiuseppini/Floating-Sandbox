/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ElectricalPanelLayoutControl.h"

#include <UILib/LayoutHelper.h>
#include <UILib/WxHelpers.h>

#include <wx/dcclient.h>

#include <algorithm>
#include <cassert>
#include <vector>

namespace ShipBuilder {

int constexpr MaxElementsPerRow = 11;

int constexpr ElementHGap = 15;
int constexpr ElementVGap = 16;
int constexpr ElementBorderThickness = 3;
int constexpr HighlightedSlotOffset = 5;
int constexpr ShadowOffset = 9;
double constexpr RoundedRectangleRadius = 9.0;

int constexpr ScrollbarHeight = 20;

ElectricalPanelLayoutControl::ElectricalPanelLayoutControl(
    wxWindow * parent,
    std::function<void(ElectricalElementInstanceIndex instanceIndex)> onElementSelected,
    ResourceLocator const & resourceLocator)
    : mElementBitmap(WxHelpers::LoadBitmap("electrical_panel_edit_element", resourceLocator))
    , mElementShadowBitmap(WxHelpers::LoadBitmap("electrical_panel_edit_element_shadow", resourceLocator))
    , mElementWidth(mElementBitmap.GetWidth())
    , mElementHeight(mElementBitmap.GetHeight())
    , mPanelHeight((ElementVGap)+(mElementHeight)+(ElementVGap)+(mElementHeight)+(ElementVGap)+ScrollbarHeight)
    //
    , mOnElementSelected(std::move(onElementSelected))
    //
    , mIsMouseCaptured(false)
    //
    , mVirtualAreaWidth(0)
    , mNElementsOnEitherSide(0)
    , mLayoutSlotsByLayoutCoordinates()
{
    // Calculate initial size
    wxSize const size = wxSize(-1, mPanelHeight);

    // Create panel
    Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        size,
        wxBORDER_SIMPLE | wxHSCROLL);

    SetBackgroundColour(*wxWHITE);

    SetMinSize(size);

    SetScrollRate(10, 0);

    // Bind paint
    Bind(wxEVT_PAINT, &ElectricalPanelLayoutControl::OnPaint, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    // Create drawing tools
    mGuidePen = wxPen(wxColor(10, 10, 10), 1, wxPENSTYLE_SHORT_DASH);
    mFreeUnselectedSlotBorderPen = wxPen(wxColor(180, 180, 180), ElementBorderThickness, wxPENSTYLE_SOLID);
    mOccupiedUnselectedSlotBorderPen = wxPen(wxColor(180, 180, 180), ElementBorderThickness, wxPENSTYLE_SOLID);
    mOccupiedSelectedSlotBorderPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID);
    mOccupiedSelectedSlotBorderBrush = wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_GRADIENTINACTIVECAPTION), wxBRUSHSTYLE_SOLID);
    mDropSlotBorderPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID);
    mDropSlotBorderBrush = wxBrush(wxColor(138, 235, 145), wxBRUSHSTYLE_SOLID);
    mTransparentPen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_TRANSPARENT);
    mTransparentBrush = wxBrush(wxColor(0, 0, 0), wxBRUSHSTYLE_TRANSPARENT);
    mInstanceIndexFont = wxFont(wxFontInfo(7));

    // Bind events
    Bind(wxEVT_CLOSE_WINDOW, &ElectricalPanelLayoutControl::OnCloseWindow, this);
    Bind(wxEVT_LEFT_DOWN, &ElectricalPanelLayoutControl::OnLeftMouseDown, this);
    Bind(wxEVT_LEFT_UP, &ElectricalPanelLayoutControl::OnLeftMouseUp, this);
    Bind(wxEVT_MOTION, &ElectricalPanelLayoutControl::OnMouseMove, this);
    Bind(wxEVT_SIZE, &ElectricalPanelLayoutControl::OnResized, this);
}

void ElectricalPanelLayoutControl::SetPanel(ElectricalPanel & electricalPanel)
{
    ResetPanel();

    mSessionData.emplace(electricalPanel);

    RecalculateLayout();

    ScrollToCenter();

    // Render
    Refresh(false);
}

void ElectricalPanelLayoutControl::ResetPanel()
{
    mSessionData.reset();
    mIsMouseCaptured = false;
    mCurrentlyMovableElement.reset();
    mCurrentlySelectedElementInstanceIndex.reset();
    mCurrentDropCandidateSlotCoordinates.reset();
}

void ElectricalPanelLayoutControl::SelectElement(ElectricalElementInstanceIndex instanceIndex)
{
    mCurrentlySelectedElementInstanceIndex = instanceIndex;

    // Render
    Refresh(false);
}

void ElectricalPanelLayoutControl::OnPanelUpdated()
{
    RecalculateLayout();

    // Render
    Refresh(false);
}

void ElectricalPanelLayoutControl::OnCloseWindow(wxCloseEvent & /*event*/)
{
    if (mIsMouseCaptured)
    {
        ReleaseMouse();
    }
}

void ElectricalPanelLayoutControl::OnLeftMouseDown(wxMouseEvent & event)
{
    if (!mIsMouseCaptured)
    {
        CaptureMouse();
        mIsMouseCaptured = true;
    }

    wxPoint const virtualCoords = ClientToVirtual(event.GetPosition());

    // Find instance index of element at this location, if any
    auto const slotCoordinates = GetSlotCoordinatesAt(virtualCoords);
    if (slotCoordinates.has_value())
    {
        assert(mLayoutSlotsByLayoutCoordinates.count(*slotCoordinates) == 1);
        LayoutSlot const & layoutSlot = mLayoutSlotsByLayoutCoordinates.at(*slotCoordinates);
        if (layoutSlot.OccupyingInstanceIndex.has_value())
        {
            // Found an element here

            auto const instanceIndex = *(layoutSlot.OccupyingInstanceIndex);

            mCurrentlyMovableElement.emplace(
                instanceIndex,
                wxPoint(
                    virtualCoords.x - layoutSlot.SlotVirtualRect.GetTopLeft().x,
                    virtualCoords.y - layoutSlot.SlotVirtualRect.GetTopLeft().y),
                virtualCoords);

            // Select it
            mCurrentlySelectedElementInstanceIndex = instanceIndex;
            mOnElementSelected(instanceIndex);

            // Render
            Refresh(false);
        }
    }
}

void ElectricalPanelLayoutControl::OnLeftMouseUp(wxMouseEvent & /*event*/)
{
    assert(mSessionData.has_value());

    if (mIsMouseCaptured)
    {
        ReleaseMouse();
        mIsMouseCaptured = false;
    }

    if (mCurrentlyMovableElement.has_value())
    {
        if (mCurrentDropCandidateSlotCoordinates.has_value())
        {
            // Get drop candidate's layout element
            assert(mLayoutSlotsByLayoutCoordinates.count(*mCurrentDropCandidateSlotCoordinates) == 1);
            LayoutSlot const & dropCandidateLayoutSlot = mLayoutSlotsByLayoutCoordinates.at(*mCurrentDropCandidateSlotCoordinates);

            if (dropCandidateLayoutSlot.OccupyingInstanceIndex.has_value())
            {
                // There is an element at the drop location...

                ElectricalElementInstanceIndex const otherElementInstanceIndex = *(dropCandidateLayoutSlot.OccupyingInstanceIndex);

                // ...move it to the layout coords of the movable element
                auto const movableElementLayoutCoords = GetLayoutCoordinatesOf(mCurrentlyMovableElement->InstanceIndex);
                mSessionData->Panel[otherElementInstanceIndex].PanelCoordinates = movableElementLayoutCoords;
                mLayoutSlotsByLayoutCoordinates.at(movableElementLayoutCoords).OccupyingInstanceIndex = otherElementInstanceIndex;
            }

            // Move movable element to drop slot
            mSessionData->Panel[mCurrentlyMovableElement->InstanceIndex].PanelCoordinates = *mCurrentDropCandidateSlotCoordinates;
            mLayoutSlotsByLayoutCoordinates.at(*mCurrentDropCandidateSlotCoordinates).OccupyingInstanceIndex = mCurrentlyMovableElement->InstanceIndex;

            RecalculateLayout();

            // Remember we're dirty
            mSessionData->IsDirty = true;
        }

        // No more movable element
        mCurrentlyMovableElement.reset();

        // No more drop candidates
        mCurrentDropCandidateSlotCoordinates.reset();

        // Render
        Refresh(false);
    }
}

void ElectricalPanelLayoutControl::OnMouseMove(wxMouseEvent & event)
{
    if (mCurrentlyMovableElement.has_value())
    {
        wxPoint const virtualCoords = ClientToVirtual(event.GetPosition());

        // Update mouse coords of currently-moving element
        mCurrentlyMovableElement->CurrentMouseCoords = virtualCoords;

        // Deselect previous slot
        mCurrentDropCandidateSlotCoordinates.reset();

        // Check if we've moved on a new candidate slot
        {
            wxPoint const movableElementCenterCoords =
                (mCurrentlyMovableElement->CurrentMouseCoords - mCurrentlyMovableElement->InRectAnchorMouseCoords) // Top, Left
                + wxSize(mElementWidth / 2, mElementHeight / 2);

            auto const slotCoordinates = GetSlotCoordinatesAt(movableElementCenterCoords);
            if (slotCoordinates.has_value())
            {
                // Select new slot, but only if it's not the origin one
                assert(mLayoutSlotsByLayoutCoordinates.count(*slotCoordinates) == 1);
                if (mLayoutSlotsByLayoutCoordinates.at(*slotCoordinates).OccupyingInstanceIndex != mCurrentlyMovableElement->InstanceIndex)
                {
                    mCurrentDropCandidateSlotCoordinates = slotCoordinates;
                }
            }
        }

        // Render
        Refresh(false);
    }
}

void ElectricalPanelLayoutControl::OnResized(wxSizeEvent & /*event*/)
{
    if (mSessionData.has_value())
    {
        RecalculateLayout();
    }

    ScrollToCenter();
}

void ElectricalPanelLayoutControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ElectricalPanelLayoutControl::Render(wxDC & dc)
{
    auto const virtualOriginX = CalcUnscrolledPosition(wxPoint(0, 0)).x;

    dc.Clear();

    //
    // Draw guides
    //

    int const guideX = (mVirtualAreaWidth / 2) - virtualOriginX;
    dc.SetPen(mGuidePen);
    dc.DrawLine(
        wxPoint(guideX, 0),
        wxPoint(guideX, mPanelHeight));

    //
    // Draw slots
    //

    for (auto const & layoutSlot : mLayoutSlotsByLayoutCoordinates)
    {
        //
        // Draw slot outline
        //

        // Check if this slot is a drop candidate
        if (mCurrentDropCandidateSlotCoordinates.has_value()
            && *mCurrentDropCandidateSlotCoordinates == layoutSlot.first)
        {
            // Drop candidate
            RenderSlot(
                layoutSlot.second.SlotVirtualRect.Inflate(HighlightedSlotOffset, HighlightedSlotOffset),
                virtualOriginX,
                mDropSlotBorderPen,
                mDropSlotBorderBrush,
                dc);
        }
        else
        {
            // Check if this coord is occupied
            if (layoutSlot.second.OccupyingInstanceIndex.has_value())
            {
                // Occupied

                if (mCurrentlySelectedElementInstanceIndex == layoutSlot.second.OccupyingInstanceIndex)
                {
                    // Selected
                    RenderSlot(
                        layoutSlot.second.SlotVirtualRect.Inflate(HighlightedSlotOffset, HighlightedSlotOffset),
                        virtualOriginX,
                        mOccupiedSelectedSlotBorderPen,
                        mOccupiedSelectedSlotBorderBrush,
                        dc);
                }
                else
                {
                    // Unselected
                    RenderSlot(
                        layoutSlot.second.SlotVirtualRect,
                        virtualOriginX,
                        mOccupiedUnselectedSlotBorderPen,
                        mTransparentBrush,
                        dc);
                }
            }
            else
            {
                // Free
                RenderSlot(
                    layoutSlot.second.SlotVirtualRect,
                    virtualOriginX,
                    mFreeUnselectedSlotBorderPen,
                    mTransparentBrush,
                    dc);
            }
        }

        //
        // Draw element (unless it's moving)
        //

        if (layoutSlot.second.OccupyingInstanceIndex.has_value())
        {
            if (!mCurrentlyMovableElement.has_value()
                || mCurrentlyMovableElement->InstanceIndex != layoutSlot.second.OccupyingInstanceIndex)
            {
                RenderElement(
                    *(layoutSlot.second.OccupyingInstanceIndex),
                    layoutSlot.second.SlotVirtualRect,
                    virtualOriginX,
                    false,
                    dc);
            }
        }
    }

    //
    // Draw movable element now
    //

    if (mCurrentlyMovableElement.has_value())
    {
        wxRect const movableElementRect = wxRect(
            mCurrentlyMovableElement->CurrentMouseCoords - mCurrentlyMovableElement->InRectAnchorMouseCoords,
            wxSize(mElementWidth, mElementHeight));

        RenderElement(
            mCurrentlyMovableElement->InstanceIndex,
            movableElementRect,
            virtualOriginX,
            true,
            dc);
    }
}

void ElectricalPanelLayoutControl::RenderSlot(
    wxRect const & virtualRect,
    int virtualOriginX,
    wxPen const & pen,
    wxBrush const & brush,
    wxDC & dc)
{
    wxRect borderDcRect = virtualRect.Inflate(1, 1);
    borderDcRect.Offset(-virtualOriginX, 0);

    dc.SetPen(pen);
    dc.SetBrush(brush);
    dc.DrawRoundedRectangle(borderDcRect, RoundedRectangleRadius);
}

void ElectricalPanelLayoutControl::RenderElement(
    ElectricalElementInstanceIndex instanceIndex,
    wxRect const & virtualRect,
    int virtualOriginX,
    bool isBeingMoved,
    wxDC & dc)
{
    wxRect elementDcRect = virtualRect;
    elementDcRect.Offset(-virtualOriginX, 0);

    if (isBeingMoved)
    {
        // Shadow

        wxPoint const topLeftShadow =
            wxPoint(
                elementDcRect.x + elementDcRect.width / 2 + ShadowOffset / 2,
                elementDcRect.y + elementDcRect.height / 2 + ShadowOffset / 2) // Center
            - wxSize(
                mElementShadowBitmap.GetWidth() / 2,
                mElementShadowBitmap.GetHeight() / 2);

        dc.DrawBitmap(
            mElementShadowBitmap,
            topLeftShadow,
            true);

        // Counter-offset element
        elementDcRect.Offset(-ShadowOffset / 2, -ShadowOffset / 2);
    }

    int const centerX = elementDcRect.GetLeft() + elementDcRect.GetWidth() / 2;

    // Draw texture
    dc.DrawBitmap(
        mElementBitmap,
        elementDcRect.GetLeftTop(),
        true);

    // Draw instance index
    wxString const instanceIndexText(std::to_string(instanceIndex));
    wxSize const instanceIndexTextSize = dc.GetTextExtent(instanceIndexText);
    dc.SetFont(mInstanceIndexFont);
    dc.SetTextForeground(wxColor(230, 230, 230));
    dc.DrawText(
        instanceIndexText,
        centerX - instanceIndexTextSize.GetWidth() / 2,
        elementDcRect.GetTop() + elementDcRect.GetHeight() / 2 - instanceIndexTextSize.GetHeight() / 2);
}

void ElectricalPanelLayoutControl::ScrollToCenter()
{
    int xUnit, yUnit;
    GetScrollPixelsPerUnit(&xUnit, &yUnit);
    if (xUnit != 0)
    {
        int const amountToScroll = std::max(
            mVirtualAreaWidth / 2 - GetSize().GetWidth() / 2,
            0);

        Scroll(
            amountToScroll / xUnit,
            -1);
    }
}

void ElectricalPanelLayoutControl::RecalculateLayout()
{
    assert(mSessionData.has_value());

    //
    // Layout and populate elements
    //

    {
        //
        // Prepare elements for layout helper
        //

        std::vector<LayoutHelper::LayoutElement<ElectricalElementInstanceIndex>> layoutElements;

        for (auto const & element : mSessionData->Panel)
        {
            auto const instanceIndex = element.first;

            if (!element.second.IsHidden)
            {
                if (element.second.PanelCoordinates.has_value())
                {
                    layoutElements.emplace_back(
                        instanceIndex,
                        *(element.second.PanelCoordinates));
                }
                else
                {
                    layoutElements.emplace_back(
                        instanceIndex,
                        std::nullopt);
                }
            }
        }

        // Sort elements by instance ID
        std::sort(
            layoutElements.begin(),
            layoutElements.end(),
            [this](auto const & lhs, auto const & rhs)
            {
                return lhs.Element < rhs.Element;
            });

        //
        // Layout
        //

        LayoutHelper::Layout<ElectricalElementInstanceIndex>(
            layoutElements,
            MaxElementsPerRow,
            [this](int nCols, int /*nRows*/)
            {
                // Calculate virtual size
                int const requiredWidth = nCols * mElementWidth + (nCols + 1) * ElementHGap;
                mVirtualAreaWidth = std::max(requiredWidth, GetSize().GetWidth() - 2);
                SetVirtualSize(mVirtualAreaWidth, mPanelHeight);

                // Calculate extent
                mNElementsOnEitherSide = ((mVirtualAreaWidth / 2) - (mElementWidth / 2 + ElementHGap)) / (mElementWidth + ElementHGap);

                //
                // Generate slots
                //

                mLayoutSlotsByLayoutCoordinates.clear();

                for (int x = -mNElementsOnEitherSide; x <= mNElementsOnEitherSide; ++x)
                {
                    for (int y = 0; y <= 1; ++y)
                    {
                        IntegralCoordinates const slotCoords{ x, y };

                        auto const [_, isInserted] = mLayoutSlotsByLayoutCoordinates.try_emplace(
                            slotCoords,
                            std::nullopt,
                            MakeSlotVirtualRect(slotCoords));

                        assert(isInserted);
                        (void)isInserted;
                    }
                }
            },
            [this](std::optional<ElectricalElementInstanceIndex> instanceIndex, IntegralCoordinates const & layoutCoords)
            {
                if (instanceIndex.has_value())
                {
                    //
                    // Store this instance at this slot, both in the panel and in the layout
                    //

                    assert(mSessionData->Panel.Contains(*instanceIndex));
                    mSessionData->Panel[*instanceIndex].PanelCoordinates = layoutCoords;

                    assert(mLayoutSlotsByLayoutCoordinates.count(layoutCoords) == 1);
                    mLayoutSlotsByLayoutCoordinates.at(layoutCoords).OccupyingInstanceIndex = instanceIndex;
                }
            });
    }
}

wxPoint ElectricalPanelLayoutControl::ClientToVirtual(wxPoint const & clientCoords) const
{
    return CalcUnscrolledPosition(clientCoords);
}

wxRect ElectricalPanelLayoutControl::ClientToVirtual(wxRect const & clientCoords) const
{
    return wxRect(
        ClientToVirtual(clientCoords.GetTopLeft()),
        clientCoords.GetSize());
}

wxRect ElectricalPanelLayoutControl::MakeSlotVirtualRect(IntegralCoordinates const & layoutCoordinates) const
{
    wxPoint const centerOfElementVirtualCoords = wxPoint(
        (mVirtualAreaWidth / 2) + layoutCoordinates.x * mElementWidth + layoutCoordinates.x * ElementHGap,
        ((mPanelHeight - ScrollbarHeight) / 2) - (mElementHeight / 2 + ElementVGap / 2) + layoutCoordinates.y * (mElementHeight + ElementVGap));

    return wxRect(
        centerOfElementVirtualCoords.x - mElementWidth / 2,
        centerOfElementVirtualCoords.y - mElementHeight / 2,
        mElementWidth,
        mElementHeight);
}

std::optional<IntegralCoordinates> ElectricalPanelLayoutControl::GetSlotCoordinatesAt(wxPoint const & virtualCoords) const
{
    int const slotWidth = mElementWidth + ElementHGap;
    int const relativeVirtualCoordsX = (virtualCoords.x - (mVirtualAreaWidth / 2));
    int const adjustedVirtualCoordsX = relativeVirtualCoordsX >= 0
        ? relativeVirtualCoordsX + slotWidth / 2
        : relativeVirtualCoordsX - slotWidth / 2;

    IntegralCoordinates layoutCoords = IntegralCoordinates(
        adjustedVirtualCoordsX / (mElementWidth + ElementHGap),
        virtualCoords.y / (ElementVGap + mElementHeight + ElementVGap / 2));

    if (layoutCoords.x >= -mNElementsOnEitherSide && layoutCoords.x <= mNElementsOnEitherSide
        && layoutCoords.y >= 0 && layoutCoords.y <= 1
        && MakeSlotVirtualRect(layoutCoords).Contains(virtualCoords))
    {
        return layoutCoords;
    }
    else
    {
        return std::nullopt;
    }
}

IntegralCoordinates const & ElectricalPanelLayoutControl::GetLayoutCoordinatesOf(ElectricalElementInstanceIndex instanceIndex) const
{
    auto const srchIt = std::find_if(
        mLayoutSlotsByLayoutCoordinates.cbegin(),
        mLayoutSlotsByLayoutCoordinates.cend(),
        [instanceIndex](auto const & entry) -> bool
        {
            return entry.second.OccupyingInstanceIndex == instanceIndex;
        });

    assert(srchIt != mLayoutSlotsByLayoutCoordinates.cend());
    return srchIt->first;
}

}