/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/panel.h>
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

    void SetDisplayUnitsSystem(UnitsSystem displayUnitsSystem);
    void SetCanvasSize(std::optional<ShipSpaceSize> canvasSize);
    void SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates);
    void SetZoom(std::optional<float> zoom);
    void SetSampledMaterial(std::optional<std::string> materialName);
    void SetMeasuredLength(std::optional<int> measuredLength);

private:

    void RefreshCanvasSize();
    void RefreshToolCoordinates();
    void RefreshZoom();
    void RefreshSampledMaterial();

private:

    // UI
    wxStaticText * mCanvasSizeStaticText;
    wxStaticText * mToolCoordinatesStaticText;
    wxStaticText * mZoomStaticText;
    wxStaticText * mSampledMaterialNameStaticText;

    // State
    UnitsSystem mDisplayUnitsSystem;
    std::optional<ShipSpaceSize> mCanvasSize;
    std::optional<ShipSpaceCoordinates> mToolCoordinates;
    std::optional<float> mZoom;
    std::optional<std::string> mSampledMaterialName;
    std::optional<int> mMeasuredLength;
};

}