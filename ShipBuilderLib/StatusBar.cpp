/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StatusBar.h"

#include <UILib/WxHelpers.h>

#include <wx/sizer.h>
#include <wx/statbmp.h>

#include <cmath>
#include <sstream>

namespace ShipBuilder {

StatusBar::StatusBar(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : wxPanel(parent)
{
    //
    // Create controls
    //

    int constexpr SpacerSizeMinor = 5;
    int constexpr SpacerSizeMajor = 10;

    wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

    hSizer->AddSpacer(SpacerSizeMinor);

    // Canvas
    {
        // Canvas size
        {
            // Icon
            {
                auto * staticBitmap = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("image_size_icon", resourceLocator));
                hSizer->Add(staticBitmap, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }

            hSizer->AddSpacer(SpacerSizeMinor);

            // Label
            {
                mCanvasSizeStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
                mCanvasSizeStaticText->SetMinSize(wxSize(65, -1));
                hSizer->Add(mCanvasSizeStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }
        }

        hSizer->AddSpacer(SpacerSizeMajor);

        // Tool coordinates
        {
            // Icon
            {
                auto * staticBitmap = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("position_icon", resourceLocator));
                hSizer->Add(staticBitmap, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }

            hSizer->AddSpacer(SpacerSizeMinor);

            // Label
            {
                mToolCoordinatesStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
                mToolCoordinatesStaticText->SetMinSize(wxSize(65, -1));
                hSizer->Add(mToolCoordinatesStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }
        }

        hSizer->AddSpacer(SpacerSizeMajor);

        // Zoom
        {
            // Icon
            {
                auto * staticBitmap = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("zoom_icon_small", resourceLocator));
                hSizer->Add(staticBitmap, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }

            hSizer->AddSpacer(SpacerSizeMinor);

            // Label
            {
                mZoomStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
                mZoomStaticText->SetMinSize(wxSize(40, -1));
                hSizer->Add(mZoomStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }
        }
    }

    hSizer->AddStretchSpacer(1);

    SetSizer(hSizer);
}

void StatusBar::SetCanvasSize(std::optional<ShipSpaceSize> canvasSize)
{
    mCanvasSize = canvasSize;
    RefreshCanvasSize();
}

void StatusBar::SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates)
{
    mToolCoordinates = coordinates;
    RefreshToolCoordinates();
}

void StatusBar::SetZoom(std::optional<float> zoom)
{
    mZoom = zoom;
    RefreshZoom();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void StatusBar::RefreshCanvasSize()
{
    std::stringstream ss;

    if (mCanvasSize.has_value())
    {
        ss << mCanvasSize->width << " x " << mCanvasSize->height;
    }

    mCanvasSizeStaticText->SetLabel(ss.str());
}

void StatusBar::RefreshToolCoordinates()
{
    std::stringstream ss;

    if (mToolCoordinates.has_value())
    {
        ss << mToolCoordinates->x << ", " << mToolCoordinates->y;
    }

    mToolCoordinatesStaticText->SetLabel(ss.str());
}

void StatusBar::RefreshZoom()
{
    std::stringstream ss;

    if (mZoom.has_value())
    {
        // ...
        // -2 = 25%
        // -1 = 50%
        // 0 = 100%
        // 1 = 200%
        // ...

        float const zoomPercentage = std::ldexp(100.0f, *mZoom);
        ss << zoomPercentage  << "%";
    }

    mZoomStaticText->SetLabel(ss.str());
}

}