/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <Core/GameTypes.h>
#include <Core/StrongTypeDef.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer, bool IsEraser>
class PencilTool : public Tool
{
public:

    virtual ~PencilTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override;
    void OnRightMouseUp() override;
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override;

protected:

    PencilTool(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

private:

    using LayerMaterialType = typename LayerTypeTraits<TLayer>::material_type;

private:

    void Leave(bool doCommitIfEngaged);

    void StartEngagement(
        ShipSpaceCoordinates const & mouseCoordinates,
        StrongTypedBool<struct IsRightMouseButton> isRightButton);

    void DoEdit(ShipSpaceCoordinates const & mouseCoordinates);

    void EndEngagement();

    void DoTempVisualization(ShipSpaceRect const & affectedRect);

    void MendTempVisualization();

    std::optional<ShipSpaceRect> CalculateApplicableRect(ShipSpaceCoordinates const & coords) const;

    int GetPencilSize() const;

    inline LayerMaterialType const * GetFillMaterial(MaterialPlaneType plane) const;

private:

    // Original layer
    typename LayerTypeTraits<TLayer>::layer_data_type mOriginalLayerClone;

    // Ship region dirtied so far with temporary visualization
    std::optional<ShipSpaceRect> mTempVisualizationDirtyShipRegion;

    struct EngagementData
    {
        // Plane of the engagement
        MaterialPlaneType const Plane;

        // Rectangle of edit operation
        std::optional<ShipSpaceRect> EditRegion;

        // Dirty state
        ModelDirtyState OriginalDirtyState;

        // Position of previous engagement (when this is second, third, etc.)
        std::optional<ShipSpaceCoordinates> PreviousEngagementPosition;

        // Position of SHIFT lock start (when exists)
        std::optional<ShipSpaceCoordinates> ShiftLockInitialPosition;

        // Direction of SHIFT lock (when exists)
        std::optional<bool> ShiftLockIsVertical;

        EngagementData(
            MaterialPlaneType plane,
            ModelDirtyState const & dirtyState,
            std::optional<ShipSpaceCoordinates> shiftLockInitialPosition)
            : Plane(plane)
            , EditRegion()
            , OriginalDirtyState(dirtyState)
            , PreviousEngagementPosition()
            , ShiftLockInitialPosition(shiftLockInitialPosition)
            , ShiftLockIsVertical()
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;

    // Whether SHIFT is currently down or not
    bool mIsShiftDown;
};

class StructuralPencilTool : public PencilTool<LayerType::Structural, false>
{
public:

    StructuralPencilTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ElectricalPencilTool : public PencilTool<LayerType::Electrical, false>
{
public:

    ElectricalPencilTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class StructuralEraserTool : public PencilTool<LayerType::Structural, true>
{
public:

    StructuralEraserTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ElectricalEraserTool : public PencilTool<LayerType::Electrical, true>
{
public:

    ElectricalEraserTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}