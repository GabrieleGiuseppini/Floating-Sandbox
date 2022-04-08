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

int constexpr PanelHeight = (ElementVGap / 2) + (ElementHeight) + (ElementVGap) + (ElementHeight) + (ElementVGap / 2);

ElectricalPanelLayoutControl::ElectricalPanelLayoutControl(
    wxWindow * parent,
    std::function<void(ElectricalElementInstanceIndex instanceIndex)> onElementSelected,
    ResourceLocator const & resourceLocator)
    : mOnElementSelected(std::move(onElementSelected))
{
    // Calculate size
    wxSize const size = wxSize(-1, PanelHeight);

    // Create panel
    Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        size,
        wxBORDER_SIMPLE);

    SetMinSize(size);

    // Bind paint
    Bind(wxEVT_PAINT, &ElectricalPanelLayoutControl::OnPaint, this);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    // Create drawing tools
    // TODOHERE
    mGuidePen = wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SHORT_DASH);
    mWaterlinePen = wxPen(wxColor(57, 127, 189), 1, wxPENSTYLE_SOLID);
    mWaterPen = wxPen(wxColor(77, 172, 255), 1, wxPENSTYLE_SOLID);
    mWaterBrush = wxBrush(mWaterPen.GetColour(), wxBRUSHSTYLE_SOLID);

    // Bind events
    Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&ElectricalPanelLayoutControl::OnLeftMouseDown, this);
    Bind(wxEVT_LEFT_UP, (wxObjectEventFunction)&ElectricalPanelLayoutControl::OnLeftMouseUp, this);
    Bind(wxEVT_MOTION, (wxObjectEventFunction)&ElectricalPanelLayoutControl::OnMouseMove, this);
}

void ElectricalPanelLayoutControl::SetPanel(ElectricalPanelMetadata const & electricalPanelMetadata)
{
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
            [this](IntegralRectSize const & rectSize)
            {
                mVirtualPanelSize = wxSize(
                    rectSize.width * ElementWidth + (rectSize.width - 1) * ElementHGap,
                    PanelHeight);
            },
            [this](std::optional<ElectricalElementInstanceIndex> instanceIndex, IntegralCoordinates const & coords)
            {
                if (instanceIndex.has_value())
                {
                    auto const [_, isInserted] = mElements.try_emplace(
                        *instanceIndex,
                        coords,
                        MakeDcRect(coords));

                    assert(isInserted);
                }
            });
    }

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

void ElectricalPanelLayoutControl::OnLeftMouseDown(wxMouseEvent & event)
{
    wxPoint const mouseCoords = event.GetPosition();

    // Find instance index, if any
    for (auto const & element : mElements)
    {
        if (element.second.DcRect.Contains(mouseCoords))
        {
            // Found!
            mCurrentlyMovableElement.emplace(
                element.first,
                wxPoint(
                    mouseCoords.x - element.second.DcRect.x,
                    mouseCoords.y - element.second.DcRect.y),
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

void ElectricalPanelLayoutControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ElectricalPanelLayoutControl::Render(wxDC & dc)
{
    wxSize const size = GetSize();

    dc.Clear();

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
            RenderElement(element.second.DcRect, dc);
        }
    }
}

void ElectricalPanelLayoutControl::RenderElement(
    wxRect const & rect,
    wxDC & dc)
{
    dc.SetPen(mWaterPen);
    dc.SetBrush(mWaterBrush);
    dc.DrawRectangle(rect);
}

wxRect ElectricalPanelLayoutControl::MakeDcRect(IntegralCoordinates const & layoutCoordinates)
{
    wxPoint const centerOfElementCoords = wxPoint(
        (mVirtualPanelSize.GetWidth() / 2) + layoutCoordinates.x * ElementWidth + layoutCoordinates.x * ElementHGap,
        (mVirtualPanelSize.GetHeight() / 2) - (ElementHGap / 2 + layoutCoordinates.y * ElementHeight + layoutCoordinates.y * ElementVGap));

    return wxRect(
        centerOfElementCoords.x - ElementWidth / 2,
        centerOfElementCoords.y - ElementHeight / 2,
        ElementWidth,
        ElementHeight);
}

}