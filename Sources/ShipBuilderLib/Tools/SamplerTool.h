/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <UILib/WxHelpers.h>

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <Core/GameTypes.h>

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
        GameAssetManager const & gameAssetManager);

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
        GameAssetManager const & gameAssetManager);
};

class ElectricalSamplerTool : public SamplerTool<LayerType::Electrical>
{
public:

    ElectricalSamplerTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class RopeSamplerTool : public SamplerTool<LayerType::Ropes>
{
public:

    RopeSamplerTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}