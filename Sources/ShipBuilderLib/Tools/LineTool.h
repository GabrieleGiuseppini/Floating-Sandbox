/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>

#include <Core/Finalizer.h>
#include <Core/GameGeometry.h>
#include <Core/GameTypes.h>
#include <Core/StrongTypeDef.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
class LineTool : public Tool
{
public:

    virtual ~LineTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override;
    void OnRightMouseUp() override;
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override;

protected:

    LineTool(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

private:

    using LayerMaterialType = typename LayerTypeTraits<TLayer>::material_type;

private:

    void Leave(bool doCommitIfEngaged);

    void StartEngagement(
        ShipSpaceCoordinates const & mouseCoordinates,
        MaterialPlaneType plane);

    void EndEngagement(ShipSpaceCoordinates const & mouseCoordinates);

    void DoEphemeralVisualization(ShipSpaceCoordinates const & mouseCoordinates);

    template<typename TVisitor>
    void DoLine(
        ShipSpaceCoordinates const & startPoint,
        ShipSpaceCoordinates const & endPoint,
        TVisitor && visitor);

    using HasEdited = StrongTypedBool<struct _HasEdited>;

    template<bool TIsForEphemeralVisualization>
    std::pair<std::optional<ShipSpaceRect>, HasEdited> TryFill(
        ShipSpaceCoordinates const & pos,
        LayerMaterialType const * fillMaterial);

    std::optional<ShipSpaceRect> CalculateApplicableRect(ShipSpaceCoordinates const & coords) const;

    int GetLineSize() const;

    inline LayerMaterialType const * GetFillMaterial(MaterialPlaneType plane) const;

private:

    // Original layer - taken at cctor and replaced after each edit operation
    typename LayerTypeTraits<TLayer>::layer_data_type mOriginalLayerClone;

    // Ephemeral visualization
    std::optional<Finalizer> mEphemeralVisualization;

    struct EngagementData
    {
        // Dirty state
        ModelDirtyState OriginalDirtyState;

        // Start point
        ShipSpaceCoordinates StartCoords;

        // Plane of the engagement
        MaterialPlaneType Plane;

        EngagementData(
            ModelDirtyState const & dirtyState,
            ShipSpaceCoordinates const & startCoords,
            MaterialPlaneType plane)
            : OriginalDirtyState(dirtyState)
            , StartCoords(startCoords)
            , Plane(plane)
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;

    // Whether SHIFT is currently down or not
    bool mIsShiftDown;
};

class StructuralLineTool : public LineTool<LayerType::Structural>
{
public:

    StructuralLineTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ElectricalLineTool : public LineTool<LayerType::Electrical>
{
public:

    ElectricalLineTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}