/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/LayerBuffers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <UILib/WxHelpers.h>

#include <GameCore/GameTypes.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
class PencilTool : public Tool
{
protected:

    PencilTool(
        ToolType toolType,
        ModelController & modelController,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);

    void Reset() override;

    void OnMouseMove(InputState const & inputState) override;
    void OnLeftMouseDown(InputState const & inputState) override;
    void OnLeftMouseUp(InputState const & inputState) override;
    void OnRightMouseDown(InputState const & inputState) override;
    void OnRightMouseUp(InputState const & inputState) override;
    void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    void OnShiftKeyUp(InputState const & /*inputState*/) override {}
    void OnMouseOut(InputState const & inputState) override;

private:

    void CheckStartEngagement(InputState const & inputState);

    void CheckEndEngagement();

    void CheckEdit(InputState const & inputState);

private:

    struct EngagementData
    {
        // Plane of the engagement
        MaterialPlaneType const Plane;

        // Clone of region
        std::unique_ptr<typename LayerTypeTraits<TLayer>::buffer_type> RegionClone;

        // Rectangle of edit operation
        ShipSpaceRect EditRegion;

        EngagementData(
            MaterialPlaneType plane,
            std::unique_ptr<typename LayerTypeTraits<TLayer>::buffer_type> && regionClone,
            ShipSpaceCoordinates const & initialPosition)
            : Plane(plane)
            , RegionClone(std::move(regionClone))
            , EditRegion(initialPosition)
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;

    wxImage mCursorImage;
};

class StructuralPencilTool : public PencilTool<LayerType::Structural>
{
public:

    StructuralPencilTool(
        ModelController & modelController,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

class ElectricalPencilTool : public PencilTool<LayerType::Electrical>
{
public:

    ElectricalPencilTool(
        ModelController & modelController,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

}