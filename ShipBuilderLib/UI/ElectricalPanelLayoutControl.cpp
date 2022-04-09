/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ElectricalPanelLayoutControl.h"

#include <UILib/LayoutHelper.h>

#include <wx/dcclient.h>

#include <algorithm>
#include <cassert>
#include <vector>

namespace ShipBuilder {

int constexpr ElementWidth = 44;
int constexpr ElementHGap = 15;
int constexpr ElementHeight = 88;
int constexpr ElementVGap = 16;

int constexpr ElementBorderThickness = 5;

int constexpr ScrollbarHeight = 20;

int constexpr PanelHeight = (ElementVGap) + (ElementHeight) + (ElementVGap) + (ElementHeight) + (ElementVGap) + ScrollbarHeight;

ElectricalPanelLayoutControl::ElectricalPanelLayoutControl(
    wxWindow * parent,
    std::function<void(ElectricalElementInstanceIndex instanceIndex)> onElementSelected,
    ResourceLocator const & resourceLocator)
    : mIsMouseCaptured(false)
    , mOnElementSelected(std::move(onElementSelected))
    , mNElementsOnEitherSide(0)
{
    // Calculate initial size
    wxSize const size = wxSize(-1, PanelHeight);

    // Create panel
    Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        size,
        wxBORDER_SIMPLE | wxHSCROLL | wxALWAYS_SHOW_SB);

    SetMinSize(size);

    SetScrollRate(5, 0);

    // Bind paint
    Bind(wxEVT_PAINT, &ElectricalPanelLayoutControl::OnPaint, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    // Create drawing tools
    mFreeUnselectedSlotBorderPen = wxPen(wxColor(140, 140, 140), ElementBorderThickness, wxPENSTYLE_SOLID);
    mOccupiedUnselectedSlotBorderPen = wxPen(wxColor(0, 18, 150), ElementBorderThickness, wxPENSTYLE_SOLID);
    mOccupiedSelectedSlotBorderPen = wxPen(wxColor(70, 206, 224), ElementBorderThickness, wxPENSTYLE_SOLID);
    mDropSlotBorderPen = wxPen(wxColor(230, 18, 39), ElementBorderThickness, wxPENSTYLE_SOLID);
    mTransparentBrush = wxBrush(wxColor(0, 0, 0), wxBRUSHSTYLE_TRANSPARENT);
    // TODOHERE
    mWaterBrush = wxBrush(wxColor(120, 20, 0), wxBRUSHSTYLE_SOLID);

    // Bind events
    Bind(wxEVT_CLOSE_WINDOW, &ElectricalPanelLayoutControl::OnCloseWindow, this);
    Bind(wxEVT_LEFT_DOWN, &ElectricalPanelLayoutControl::OnLeftMouseDown, this);
    Bind(wxEVT_LEFT_UP, &ElectricalPanelLayoutControl::OnLeftMouseUp, this);
    Bind(wxEVT_MOTION, &ElectricalPanelLayoutControl::OnMouseMove, this);
    Bind(wxEVT_SIZE, &ElectricalPanelLayoutControl::OnResized, this);
}

void ElectricalPanelLayoutControl::SetPanel(ElectricalPanelMetadata const & electricalPanelMetadata)
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
        for (auto const & entry : electricalPanelMetadata)
        {
            // Ignore if hidden
            if (!entry.second.IsHidden)
            {
                if (entry.second.PanelCoordinates.has_value())
                {
                    layoutElements.emplace_back(
                        entry.first,
                        *(entry.second.PanelCoordinates));
                }
                else
                {
                    layoutElements.emplace_back(
                        entry.first,
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
    LogMessage("TODOHERE: OnCloseWindow");

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

    wxPoint const mouseCoords = event.GetPosition();

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

        // Render
        Refresh(false);
    }
}

void ElectricalPanelLayoutControl::OnResized(wxSizeEvent & event)
{
    LogMessage("TODOTEST: OnResized: ", event.GetSize().GetWidth(), ", ", event.GetSize().GetHeight());

    RecalculateGeometry();
}

void ElectricalPanelLayoutControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ElectricalPanelLayoutControl::Render(wxDC & dc)
{
    wxSize const size = GetSize();

    dc.Clear();

    //
    // Draw slots
    //

    for (int x = -mNElementsOnEitherSide; x <= mNElementsOnEitherSide; ++x)
    {
        for (int y = 0; y <= 1; ++y)
        {
            IntegralCoordinates const slotCoords{ x, y };

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
                        MakeDcRect(slotCoords, mVirtualAreaWidth),
                        dc);
                }
                else
                {
                    // Unselected
                    RenderSlot(
                        mOccupiedUnselectedSlotBorderPen,
                        MakeDcRect(slotCoords, mVirtualAreaWidth),
                        dc);
                }
            }
            else
            {
                // Free
                RenderSlot(
                    mFreeUnselectedSlotBorderPen,
                    MakeDcRect(slotCoords, mVirtualAreaWidth),
                    dc);
            }
        }
    }

    //
    // Draw elements
    //

    for (auto const & element : mElements)
    {
        if (mCurrentlyMovableElement.has_value()
            && mCurrentlyMovableElement->InstanceIndex == element.first)
        {
            RenderElement(
                wxRect(
                    mCurrentlyMovableElement->CurrentMouseCoords - mCurrentlyMovableElement->InRectAnchorMouseCoords,
                    wxSize(ElementWidth, ElementHeight)),
                dc);
        }
        else
        {
            assert(element.second.DcRect.has_value());
            RenderElement(*element.second.DcRect, dc);
        }
    }
}

void ElectricalPanelLayoutControl::RenderSlot(
    wxPen const & pen,
    wxRect && rect,
    wxDC & dc)
{
    rect.Inflate(ElementBorderThickness / 2, ElementBorderThickness / 2);

    dc.SetPen(pen);
    dc.SetBrush(mTransparentBrush);
    dc.DrawRectangle(rect);
}

void ElectricalPanelLayoutControl::RenderElement(
    wxRect const & rect,
    wxDC & dc)
{
    // TODOHERE
    dc.SetPen(wxPen(mWaterBrush.GetColour(), 1));
    dc.SetBrush(mWaterBrush);
    dc.DrawRectangle(rect);
}

void ElectricalPanelLayoutControl::RecalculateGeometry()
{
    // Calculate virtual size
    int const requiredWidth = mLayoutCols * ElementWidth + (mLayoutCols + 1) * ElementHGap;
    mVirtualAreaWidth = std::max(requiredWidth, GetSize().GetWidth());
    SetVirtualSize(mVirtualAreaWidth, PanelHeight);

    // Scroll to center
    // TODOHERE

    // Calculate extent
    mNElementsOnEitherSide = ((mVirtualAreaWidth / 2) - (ElementWidth / 2 + ElementHGap)) / (ElementWidth + ElementHGap);

    //
    // Calculate DC rects for all elements
    //

    for (auto & element : mElements)
    {
        element.second.DcRect = MakeDcRect(
            element.second.LayoutCoordinates,
            mVirtualAreaWidth);
    }
}

wxRect ElectricalPanelLayoutControl::MakeDcRect(
    IntegralCoordinates const & layoutCoordinates,
    int virtualAreaWidth)
{
    wxPoint const centerOfElementVirtualCoords = wxPoint(
        (virtualAreaWidth / 2) + layoutCoordinates.x * ElementWidth + layoutCoordinates.x * ElementHGap,
        ((PanelHeight - ScrollbarHeight) / 2) - (ElementHeight / 2 + ElementVGap / 2) + layoutCoordinates.y * (ElementHeight + ElementVGap));

    return wxRect(
        centerOfElementVirtualCoords.x - ElementWidth / 2,
        centerOfElementVirtualCoords.y - ElementHeight / 2,
        ElementWidth,
        ElementHeight);
}

}