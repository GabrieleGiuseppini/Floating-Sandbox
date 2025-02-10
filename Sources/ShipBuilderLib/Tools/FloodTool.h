/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-31
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <UILib/WxHelpers.h>

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <Core/GameTypes.h>
#include <Core/StrongTypeDef.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayerType>
class FloodTool : public Tool
{
public:

    virtual ~FloodTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override {};
    void OnRightMouseDown() override;
    void OnRightMouseUp() override {};
    void OnShiftKeyDown() override {};
    void OnShiftKeyUp() override {};
    void OnMouseLeft() override;

protected:

    FloodTool(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

private:

    using LayerMaterialType = typename LayerTypeTraits<TLayerType>::material_type;

private:

    void DoEdit(
        ShipSpaceCoordinates const & mouseCoordinates,
        StrongTypedBool<struct IsRightMouseButton> isRightButton);

    inline LayerMaterialType const * GetFloodMaterial(MaterialPlaneType plane) const;
};

class StructuralFloodTool : public FloodTool<LayerType::Structural>
{
public:

    StructuralFloodTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}