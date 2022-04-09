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

int constexpr ElementHGap = 15;
int constexpr ElementVGap = 16;

int constexpr ElementBorderThickness = 5;

int constexpr ScrollbarHeight = 20;

ElectricalPanelLayoutControl::ElectricalPanelLayoutControl(
    wxWindow * parent,
    std::function<void(ElectricalElementInstanceIndex instanceIndex)> onElementSelected,
    ResourceLocator const & resourceLocator)
    : mElementBitmap(WxHelpers::LoadBitmap("electrical_panel_edit_element", resourceLocator))
    , mElementWidth(mElementBitmap.GetWidth())
    , mElementHeight(mElementBitmap.GetHeight())
    , mPanelHeight((ElementVGap)+(mElementHeight)+(ElementVGap)+(mElementHeight)+(ElementVGap)+ScrollbarHeight)
    , mIsMouseCaptured(false)
    , mOnElementSelected(std::move(onElementSelected))
    , mNElementsOnEitherSide(0)
{
    // Calculate initial size
    wxSize const size = wxSize(-1, mPanelHeight);

    // Create panel
    Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        size,
        wxBORDER_SIMPLE | wxHSCROLL | wxALWAYS_SHOW_SB);

    SetMinSize(size);

    SetScrollRate(1, 0);

    // Bind paint
    Bind(wxEVT_PAINT, &ElectricalPanelLayoutControl::OnPaint, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    // Create drawing tools
    mGuidePen = wxPen(wxColor(10, 10, 10), 1, wxPENSTYLE_SHORT_DASH);
    mFreeUnselectedSlotBorderPen = wxPen(wxColor(140, 140, 140), ElementBorderThickness, wxPENSTYLE_SOLID);
    mOccupiedUnselectedSlotBorderPen = wxPen(wxColor(0, 18, 150), ElementBorderThickness, wxPENSTYLE_SOLID);
    mOccupiedSelectedSlotBorderPen = wxPen(wxColor(70, 206, 224), ElementBorderThickness, wxPENSTYLE_SOLID);
    mDropSlotBorderPen = wxPen(wxColor(230, 18, 39), ElementBorderThickness, wxPENSTYLE_SOLID);
    mTransparentBrush = wxBrush(wxColor(0, 0, 0), wxBRUSHSTYLE_TRANSPARENT);

    // Bind events
    Bind(wxEVT_CLOSE_WINDOW, &ElectricalPanelLayoutControl::OnCloseWindow, this);
    Bind(wxEVT_LEFT_DOWN, &ElectricalPanelLayoutControl::OnLeftMouseDown, this);
    Bind(wxEVT_LEFT_UP, &ElectricalPanelLayoutControl::OnLeftMouseUp, this);
    Bind(wxEVT_MOTION, &ElectricalPanelLayoutControl::OnMouseMove, this);
    Bind(wxEVT_SIZE, &ElectricalPanelLayoutControl::OnResized, this);
}

void ElectricalPanelLayoutControl::SetPanel(
    InstancedElectricalElementSet const & instancedElectricalElementSet,
    ElectricalPanelMetadata const & electricalPanelMetadata)
{
    // Reset state
    mIsMouseCaptured = false;
    mElements.clear();
    mCurrentlyMovableElement.reset();
    mCurrentlySelectedElementInstanceIndex.reset();

    // Layout and populate elements
    {
        // Prepare elements for layout helper

        std::vector<LayoutHelper::LayoutElement<ElectricalElementInstanceIndex>> layoutElements;
        for (auto const & element : instancedElectricalElementSet.GetElements())
        {
            auto const instanceIndex = element.first;

            // See if we have panel metadata for it
            auto const panelIt = electricalPanelMetadata.find(instanceIndex);
            if (panelIt != electricalPanelMetadata.cend())
            {
                if (!panelIt->second.IsHidden)
                {
                    if (panelIt->second.PanelCoordinates.has_value())
                    {
                        layoutElements.emplace_back(
                            instanceIndex,
                            *(panelIt->second.PanelCoordinates));
                    }
                    else
                    {
                        layoutElements.emplace_back(
                            instanceIndex,
                            std::nullopt);
                    }
                }
            }
            else
            {
                layoutElements.emplace_back(
                    instanceIndex,
                    std::nullopt);
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

        // Layout
        LayoutHelper::Layout<ElectricalElementInstanceIndex>(
            layoutElements,
            11, // TODO
            [this](int nCols, int nRows)
            {
                mLayoutCols = nCols;
                mLayoutRows = nRows;
            },
            [this](std::optional<ElectricalElementInstanceIndex> instanceIndex, IntegralCoordinates const & coords)
            {
                if (instanceIndex.has_value())
                {
                    auto const [_, isInserted] = mElements.try_emplace(
                        *instanceIndex,
                        coords);

                    assert(isInserted);
                }
            });
    }

    // Recalculate everything
    RecalculateGeometry();

    // Render
    Refresh(false);
}

void ElectricalPanelLayoutControl::SelectElement(ElectricalElementInstanceIndex instanceIndex)
{
    mCurrentlySelectedElementInstanceIndex = instanceIndex;

    // Render
    Refresh(false);
}

void ElectricalPanelLayoutControl::SetElementVisible(
    ElectricalElementInstanceIndex instanceIndex,
    ElectricalPanelElementMetadata const & panelMetadata,
    bool isVisible)
{
    if (isVisible)
    {
        // TODO: add element to map, and re-layout
    }
    else
    {
        assert(mElements.count(instanceIndex) == 1);
        mElements.erase(instanceIndex);
    }

    // Render
    Refresh(false);
}

void ElectricalPanelLayoutControl::OnCloseWindow(wxCloseEvent & event)
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

    wxPoint mouseCoords = event.GetPosition();
    mouseCoords.x += GetOriginVirtualX();

    // Find instance index, if any
    for (auto const & element : mElements)
    {
        assert(element.second.DcRect.has_value());

        if (element.second.DcRect->Contains(mouseCoords))
        {
            // Found!
            mCurrentlyMovableElement.emplace(
                element.first,
                wxPoint(
                    mouseCoords.x - element.second.DcRect->x,
                    mouseCoords.y - element.second.DcRect->y),
                mouseCoords);

            // Select it
            mCurrentlySelectedElementInstanceIndex = element.first;
            mOnElementSelected(element.first);

            // Render
            Refresh(false);

            return;
        }
    }
}

void ElectricalPanelLayoutControl::OnLeftMouseUp(wxMouseEvent & event)
{
    if (mIsMouseCaptured)
    {
        ReleaseMouse();
        mIsMouseCaptured = false;
    }

    if (mCurrentlyMovableElement.has_value())
    {
        // TODO: finalize move

        // No more movable element
        mCurrentlyMovableElement.reset();

        // Render
        Refresh(false);
    }
}

void ElectricalPanelLayoutControl::OnMouseMove(wxMouseEvent & event)
{
    if (mCurrentlyMovableElement.has_value())
    {
        // Update mouse coords of currently-moving element
        mCurrentlyMovableElement->CurrentMouseCoords = event.GetPosition();
        mCurrentlyMovableElement->CurrentMouseCoords.x += GetOriginVirtualX();

        // Render
        Refresh(false);
    }
}

void ElectricalPanelLayoutControl::OnResized(wxSizeEvent & event)
{
    RecalculateGeometry();
}

void ElectricalPanelLayoutControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ElectricalPanelLayoutControl::Render(wxDC & dc)
{
    auto const virtualOriginX = GetOriginVirtualX();

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

    for (int x = -mNElementsOnEitherSide; x <= mNElementsOnEitherSide; ++x)
    {
        for (int y = 0; y <= 1; ++y)
        {
            IntegralCoordinates const slotCoords{ x, y };

            wxRect const slotRect = MakeDcRect(slotCoords);

            // Check if this coord is occupied

            auto const srchIt = std::find_if(
                mElements.cbegin(),
                mElements.cend(),
                [this, &slotCoords](auto const & entry) -> bool
                {
                    return entry.second.LayoutCoordinates == slotCoords;
                });

            if (srchIt != mElements.cend())
            {
                // Occupied

                if (mCurrentlySelectedElementInstanceIndex.has_value()
                    && *mCurrentlySelectedElementInstanceIndex == srchIt->first)
                {
                    // Selected
                    RenderSlot(
                        mOccupiedSelectedSlotBorderPen,
                        slotRect,
                        virtualOriginX,
                        dc);
                }
                else
                {
                    // Unselected
                    RenderSlot(
                        mOccupiedUnselectedSlotBorderPen,
                        slotRect,
                        virtualOriginX,
                        dc);
                }
            }
            else
            {
                // Free
                RenderSlot(
                    mFreeUnselectedSlotBorderPen,
                    slotRect,
                    virtualOriginX,
                    dc);
            }
        }
    }

    //
    // Draw elements
    //

    for (auto const & element : mElements)
    {
        if (!mCurrentlyMovableElement.has_value()
            || mCurrentlyMovableElement->InstanceIndex != element.first)
        {
            assert(element.second.DcRect.has_value());

            RenderElement(
                *element.second.DcRect,
                virtualOriginX,
                dc);
        }
    }

    if (mCurrentlyMovableElement.has_value())
    {
        RenderElement(
            wxRect(
                mCurrentlyMovableElement->CurrentMouseCoords - mCurrentlyMovableElement->InRectAnchorMouseCoords,
                wxSize(mElementWidth, mElementHeight)),
            virtualOriginX,
            dc);
    }
}

void ElectricalPanelLayoutControl::RenderSlot(
    wxPen const & pen,
    wxRect const & rect,
    int virtualOriginX,
    wxDC & dc)
{
    //wxRect borderRect = rect.Inflate(ElementBorderThickness / 2, ElementBorderThickness / 2);
    wxRect borderRect = rect.Inflate(1, 1);
    borderRect.Offset(-virtualOriginX, 0);

    dc.SetPen(pen);
    dc.SetBrush(mTransparentBrush);
    dc.DrawRectangle(borderRect);
}

void ElectricalPanelLayoutControl::RenderElement(
    wxRect const & rect,
    int virtualOriginX,
    wxDC & dc)
{
    wxRect elementRect = rect;
    elementRect.Offset(-virtualOriginX, 0);

    dc.DrawBitmap(
        mElementBitmap,
        elementRect.GetLeftTop(),
        true);
}

void ElectricalPanelLayoutControl::RecalculateGeometry()
{
    // Calculate virtual size
    int const requiredWidth = mLayoutCols * mElementWidth + (mLayoutCols + 1) * ElementHGap;
    int const visibleWidth = GetSize().GetWidth();
    mVirtualAreaWidth = std::max(requiredWidth, visibleWidth);
    SetVirtualSize(mVirtualAreaWidth, mPanelHeight);

    // Scroll to H center
    {
        int xUnit, yUnit;
        GetScrollPixelsPerUnit(&xUnit, &yUnit);
        if (xUnit != 0)
        {
            int const amountToScroll = std::max(
                mVirtualAreaWidth / 2 - visibleWidth / 2,
                0);

            Scroll(
                amountToScroll / xUnit,
                -1);
        }
    }

    // Calculate extent
    mNElementsOnEitherSide = ((mVirtualAreaWidth / 2) - (mElementWidth / 2 + ElementHGap)) / (mElementWidth + ElementHGap);

    //
    // Calculate DC rects for all elements
    //

    for (auto & element : mElements)
    {
        element.second.DcRect = MakeDcRect(element.second.LayoutCoordinates);
    }
}

wxRect ElectricalPanelLayoutControl::MakeDcRect(IntegralCoordinates const & layoutCoordinates) const
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

int ElectricalPanelLayoutControl::GetOriginVirtualX() const
{
    return CalcUnscrolledPosition(wxPoint(0, 0)).x;
}

}