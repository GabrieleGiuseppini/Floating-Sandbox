/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <UILib/WxHelpers.h>

#include <GameCore/GameTypes.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
class SamplerTool : public Tool
{
public:

    virtual ~SamplerTool();

    void OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override {};
    void OnRightMouseDown() override;
    void OnRightMouseUp() override {};
    void OnShiftKeyDown() override {};
    void OnShiftKeyUp() override {};
    void OnMouseLeft() override;

protected:

    SamplerTool(
        ToolType toolType,
        Controller & controller,
        ResourceLocator const & resourceLocator);

private:

    using LayerMaterialType = typename LayerTypeTraits<TLayer>::material_type;

private:

    void DoSelectMaterial(
        ShipSpaceCoordinates const & mouseCoordinates,
        MaterialPlaneType plane);

    inline LayerMaterialType const * SampleMaterial(ShipSpaceCoordinates const & mouseCoordinates);
};

class StructuralSamplerTool : public SamplerTool<LayerType::Structural>
{
public:

    StructuralSamplerTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class ElectricalSamplerTool : public SamplerTool<LayerType::Electrical>
{
public:

    ElectricalSamplerTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class RopeSamplerTool : public SamplerTool<LayerType::Ropes>
{
public:

    RopeSamplerTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

}