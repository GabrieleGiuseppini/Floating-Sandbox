/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StatusBar.h"

#include <UILib/WxHelpers.h>

#include <GameCore/Conversions.h>

#include <wx/sizer.h>
#include <wx/statline.h>

#include <cmath>
#include <iomanip>
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

        // Selection size
        {
            // Icon
            {
                auto * staticBitmap = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("selection_size_icon", resourceLocator));
                hSizer->Add(staticBitmap, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }

            hSizer->AddSpacer(SpacerSizeMinor);

            // Label
            {
                mSelectionSizeStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
                mSelectionSizeStaticText->SetMinSize(wxSize(70, -1));
                hSizer->Add(mSelectionSizeStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }
        }

        hSizer->AddSpacer(SpacerSizeMajor);

        // Sampled data
        {
            // Icon
            {
                auto * staticBitmap = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("sampler_icon_small", resourceLocator));
                hSizer->Add(staticBitmap, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }

            hSizer->AddSpacer(SpacerSizeMinor);

            // Label
            {
                mSampledInformationStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
                mSampledInformationStaticText->SetMinSize(wxSize(200, -1));
                hSizer->Add(mSampledInformationStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
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

    hSizer->AddSpacer(SpacerSizeMinor);

    // Separator
    {
        auto * line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
        hSizer->Add(line, 0, wxEXPAND, 0);
    }

    hSizer->AddSpacer(SpacerSizeMinor);

    // Ship
    {
        // Mass
        {
            // Icon
            {
                auto * staticBitmap = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("weight_icon_small", resourceLocator));
                hSizer->Add(staticBitmap, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }

            hSizer->AddSpacer(SpacerSizeMinor);

            // Label
            {
                mShipMassStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
                mShipMassStaticText->SetMinSize(wxSize(60, -1));
                hSizer->Add(mShipMassStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
            }
        }
    }

    hSizer->AddStretchSpacer(1);

    // Separator
    {
        auto * line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
        hSizer->Add(line, 0, wxEXPAND, 0);
    }

    hSizer->AddSpacer(SpacerSizeMinor);

    // Current tool icon
    {
        mCurrentToolStaticBitmap = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
        mCurrentToolStaticBitmap->SetMinSize(wxSize(16, 16));
        hSizer->Add(mCurrentToolStaticBitmap, 0, wxALIGN_CENTRE_VERTICAL, 0);
    }

    hSizer->AddSpacer(SpacerSizeMinor);

    // Tool output label
    {
        mToolOutputStaticText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
        mToolOutputStaticText->SetMinSize(wxSize(200, -1));
        hSizer->Add(mToolOutputStaticText, 0, wxALIGN_CENTRE_VERTICAL, 0);
    }

    SetSizer(hSizer);

    //
    // Load bitmaps
    //

    mMeasuringTapeToolBitmap = WxHelpers::LoadBitmap("measuring_tape_icon_small", resourceLocator);
}

void StatusBar::SetShipScale(ShipSpaceToWorldSpaceCoordsRatio scale)
{
    if (!mShipScale.has_value() || scale != *mShipScale)
    {
        mShipScale = scale;

        // Refresh all labels affected by scale
        RefreshCanvasSize();
        RefreshToolOutput();
    }
}

void StatusBar::SetDisplayUnitsSystem(UnitsSystem displayUnitsSystem)
{
    if (displayUnitsSystem != mDisplayUnitsSystem)
    {
        mDisplayUnitsSystem = displayUnitsSystem;

        // Refresh all labels affected by units system
        RefreshCanvasSize();
        RefreshShipMass();
        RefreshToolOutput();
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

void StatusBar::SetSelectionSize(std::optional<ShipSpaceSize> selectionSize)
{
    if (selectionSize != mSelectionSize)
    {
        mSelectionSize = selectionSize;
        RefreshSelectionSize();
    }
}

void StatusBar::SetSampledInformation(std::optional<SampledInformation> sampledInformation)
{
    if (!(sampledInformation == mSampledInformation))
    {
        mSampledInformation = sampledInformation;
        RefreshSampledInformation();
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

void StatusBar::SetShipMass(std::optional<float> shipMass)
{
    if (shipMass != mShipMass)
    {
        mShipMass = shipMass;
        RefreshShipMass();
    }
}
void StatusBar::SetCurrentToolType(ToolType toolType)
{
    if (toolType != mCurrentToolType)
    {
        mCurrentToolType = toolType;
        RefreshCurrentToolType();
    }
}

void StatusBar::SetMeasuredWorldLength(std::optional<int> measuredWorldLength)
{
    if (measuredWorldLength != mMeasuredWorldLength)
    {
        mMeasuredWorldLength = measuredWorldLength;
        RefreshToolOutput();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void StatusBar::RefreshCanvasSize()
{
    std::stringstream ss;

    if (mCanvasSize.has_value())
    {
        ss << mCanvasSize->width << " x " << mCanvasSize->height;

        if (mShipScale.has_value())
        {
            auto const worldCoords = mCanvasSize->ToFractionalCoords(*mShipScale);

            ss << " (";

            switch (mDisplayUnitsSystem)
            {
                case UnitsSystem::SI_Celsius:
                case UnitsSystem::SI_Kelvin:
                {
                    ss << worldCoords.x << " x " << worldCoords.y
                        << " m";
                    break;
                }

                case UnitsSystem::USCS:
                {
                    ss << std::round(Conversions::MeterToFoot(worldCoords.x)) << " x " << std::round(Conversions::MeterToFoot(worldCoords.y))
                        << " ft";
                    break;
                }
            }

            ss << ")";
        }
    }

    mCanvasSizeStaticText->SetLabel(ss.str());
}

void StatusBar::RefreshToolCoordinates()
{
    std::stringstream ss;

    if (mToolCoordinates.has_value())
    {
        ss << mToolCoordinates->x << ", " << mToolCoordinates->y;

        if (mShipScale.has_value())
        {
            auto const worldCoords = mToolCoordinates->ToFractionalCoords(*mShipScale);

            ss << " (";

            switch (mDisplayUnitsSystem)
            {
                case UnitsSystem::SI_Celsius:
                case UnitsSystem::SI_Kelvin:
                {
                    ss << worldCoords.x << ", " << worldCoords.y
                        << " m";
                    break;
                }

                case UnitsSystem::USCS:
                {
                    ss << std::round(Conversions::MeterToFoot(worldCoords.x)) << ", " << std::round(Conversions::MeterToFoot(worldCoords.y))
                        << " ft";
                    break;
                }
            }

            ss << ")";
        }
    }

    mToolCoordinatesStaticText->SetLabel(ss.str());
}

void StatusBar::RefreshSelectionSize()
{
    std::stringstream ss;

    if (mSelectionSize.has_value())
    {
        ss << mSelectionSize->width << " x " << mSelectionSize->height;
    }

    mSelectionSizeStaticText->SetLabel(ss.str());
}

void StatusBar::RefreshSampledInformation()
{
    std::stringstream ss;

    if (mSampledInformation.has_value())
    {
        ss << mSampledInformation->MaterialName;

        if (mSampledInformation->InstanceIndex.has_value())
        {
            ss << " (" << *(mSampledInformation->InstanceIndex) << ")";
        }
    }

    mSampledInformationStaticText->SetLabel(ss.str());
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

void StatusBar::RefreshShipMass()
{
    std::stringstream ss;

    ss << std::fixed << std::setprecision(1);

    float mass = mShipMass.value_or(0.0f);

    switch (mDisplayUnitsSystem)
    {
        case UnitsSystem::SI_Celsius:
        case UnitsSystem::SI_Kelvin:
        {
            ss << Conversions::KilogramToMetricTon(mass)
                << " t";
            break;
        }

        case UnitsSystem::USCS:
        {
            ss << Conversions::KilogramToUscsTon(mass)
                << " tn";
            break;
        }
    }

    mShipMassStaticText->SetLabel(ss.str());
}

void StatusBar::RefreshCurrentToolType()
{
    wxBitmap bitmap = wxNullBitmap;

    if (mCurrentToolType.has_value())
    {
        switch (*mCurrentToolType)
        {
            case ToolType::StructuralMeasuringTapeTool:
            {
                bitmap = mMeasuringTapeToolBitmap;
                break;
            }

            default:
            {
                // No icon
                break;
            }
        }
    }

    mCurrentToolStaticBitmap->SetBitmap(bitmap);
}

void StatusBar::RefreshToolOutput()
{
    std::stringstream ss;

    if (mCurrentToolType.has_value())
    {
        switch (*mCurrentToolType)
        {
            case ToolType::StructuralMeasuringTapeTool:
            {
                if (mMeasuredWorldLength.has_value())
                {
                    ss << *mMeasuredWorldLength
                        << " (";

                    switch (mDisplayUnitsSystem)
                    {
                        case UnitsSystem::SI_Celsius:
                        case UnitsSystem::SI_Kelvin:
                        {
                            ss << *mMeasuredWorldLength
                                << " m";
                            break;
                        }

                        case UnitsSystem::USCS:
                        {
                            ss << std::round(Conversions::MeterToFoot(*mMeasuredWorldLength))
                                << " ft";
                            break;
                        }
                    }

                    ss << ")";
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    mToolOutputStaticText->SetLabel(ss.str());
}

}