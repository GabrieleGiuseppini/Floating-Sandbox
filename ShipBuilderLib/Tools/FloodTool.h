/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-31
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <UILib/WxHelpers.h>

#include <GameCore/GameTypes.h>
#include <GameCore/StrongTypeDef.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
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
        ResourceLocator const & resourceLocator);

private:

    using LayerMaterialType = typename LayerTypeTraits<TLayer>::material_type;

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
        ResourceLocator const & resourceLocator);
};

}