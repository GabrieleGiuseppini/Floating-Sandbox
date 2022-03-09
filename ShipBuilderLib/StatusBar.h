/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/panel.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <optional>

namespace ShipBuilder {

class StatusBar : public wxPanel
{
public:

    StatusBar(
        wxWindow * parent,
        UnitsSystem displayUnitsSystem,
        ResourceLocator const & resourceLocator);

    void SetShipScale(ShipSpaceToWorldSpaceCoordsRatio scale);
    void SetDisplayUnitsSystem(UnitsSystem displayUnitsSystem);

    void SetCanvasSize(std::optional<ShipSpaceSize> canvasSize);
    void SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates);
    void SetZoom(std::optional<float> zoom);
    void SetShipMass(std::optional<float> shipMass);
    void SetCurrentToolType(std::optional<ToolType> toolType);
    void SetSampledMaterial(std::optional<std::string> materialName);
    void SetMeasuredWorldLength(std::optional<int> measuredWorldLength);

private:

    void RefreshCanvasSize();
    void RefreshToolCoordinates();
    void RefreshZoom();
    void RefreshShipMass();
    void RefreshCurrentToolType();
    void RefreshToolOutput();

private:

    // UI
    wxStaticText * mCanvasSizeStaticText;
    wxStaticText * mToolCoordinatesStaticText;
    wxStaticText * mZoomStaticText;
    wxStaticText * mShipMassStaticText;
    wxStaticBitmap * mCurrentToolStaticBitmap;
    wxBitmap mSamplerToolBitmap;
    wxBitmap mMeasuringTapeToolBitmap;
    wxStaticText * mToolOutputStaticText;

    // State
    std::optional<ShipSpaceToWorldSpaceCoordsRatio> mShipScale;
    UnitsSystem mDisplayUnitsSystem;
    std::optional<ShipSpaceSize> mCanvasSize;
    std::optional<ShipSpaceCoordinates> mToolCoordinates;
    std::optional<float> mZoom;
    std::optional<float> mShipMass;
    std::optional<ToolType> mCurrentToolType;
    std::optional<std::string> mSampledMaterialName;
    std::optional<int> mMeasuredWorldLength;
};

}