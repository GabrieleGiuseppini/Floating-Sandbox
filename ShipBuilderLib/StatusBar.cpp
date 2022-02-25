/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StatusBar.h"

#include <UILib/WxHelpers.h>

#include <GameCore/Conversions.h>

#include <wx/sizer.h>
#include <wx/statbmp.h>

#include <cmath>
#include <sstream>

namespace ShipBuilder {

StatusBar::StatusBar(
    wxWindow * parent,
    UnitsSystem displayUnitsSystem,
    ResourceLocator const & resourceLocator)
    : wxPanel(parent)
    , mDisplayUnitsSystem(displayUnitsSystem)
{
    //
    // Create controls
    //

    int constexpr SpacerSizeMinor = 5;
    int constexpr SpacerSizeMajor = 15;

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
                mCanvasSizeStaticText->SetMinSize(wxSize(160, -1));
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
                mToolCoordinatesStaticText->SetMinSize(wxSize(140, -1));
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

    // Sampled material name
    {
        mSampledMaterialNameStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
        mSampledMaterialNameStaticText->SetMinSize(wxSize(200, -1));
        hSizer->Add(mSampledMaterialNameStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
    }

    SetSizer(hSizer);
}

void StatusBar::SetDisplayUnitsSystem(UnitsSystem displayUnitsSystem)
{
    if (displayUnitsSystem != mDisplayUnitsSystem)
    {
        mDisplayUnitsSystem = displayUnitsSystem;

        // Refresh all labels affected by units system
        RefreshCanvasSize();
    }
}
void StatusBar::SetCanvasSize(std::optional<ShipSpaceSize> canvasSize)
{
    if (canvasSize != mCanvasSize)
    {
        mCanvasSize = canvasSize;
        RefreshCanvasSize();
    }
}

void StatusBar::SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates)
{
    if (coordinates != mToolCoordinates)
    {
        mToolCoordinates = coordinates;
        RefreshToolCoordinates();
    }
}

void StatusBar::SetZoom(std::optional<float> zoom)
{
    if (zoom != mZoom)
    {
        mZoom = zoom;
        RefreshZoom();
    }
}

void StatusBar::SetSampledMaterial(std::optional<std::string> materialName)
{
    if (materialName != mSampledMaterialName)
    {
        mSampledMaterialName = materialName;
        // TODOHERE
        RefreshSampledMaterial();
    }
}

void StatusBar::SetMeasuredLength(std::optional<int> measuredLength)
{
    if (measuredLength != mMeasuredLength)
    {
        mMeasuredLength = measuredLength;
        // TODOHERE
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void StatusBar::RefreshCanvasSize()
{
    std::stringstream ss;

    if (mCanvasSize.has_value())
    {
        ss << mCanvasSize->width << " x " << mCanvasSize->height
            << " (";

        switch (mDisplayUnitsSystem)
        {
            case UnitsSystem::SI_Celsius:
            case UnitsSystem::SI_Kelvin:
            {
                ss << mCanvasSize->width << " x " << mCanvasSize->height
                    << " m";
                break;
            }

            case UnitsSystem::USCS:
            {
                ss << std::round(MetersToFeet(mCanvasSize->width)) << " x " << std::round(MetersToFeet(mCanvasSize->height))
                    << " ft";
                break;
            }
        }

        ss << ")";
    }

    mCanvasSizeStaticText->SetLabel(ss.str());
}

void StatusBar::RefreshToolCoordinates()
{
    std::stringstream ss;

    if (mToolCoordinates.has_value())
    {
        ss << mToolCoordinates->x << ", " << mToolCoordinates->y
            << " (";

        switch (mDisplayUnitsSystem)
        {
            case UnitsSystem::SI_Celsius:
            case UnitsSystem::SI_Kelvin:
            {
                ss << mToolCoordinates->x << ", " << mToolCoordinates->y
                    << " m";
                break;
            }

            case UnitsSystem::USCS:
            {
                ss << std::round(MetersToFeet(mToolCoordinates->x)) << ", " << std::round(MetersToFeet(mToolCoordinates->y))
                    << " ft";
                break;
            }
        }

        ss << ")";
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

void StatusBar::RefreshSampledMaterial()
{
    mSampledMaterialNameStaticText->SetLabel(mSampledMaterialName.value_or(""));
}

}