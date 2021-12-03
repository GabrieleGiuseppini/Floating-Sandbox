/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LineTool.h"

#include "Controller.h"

#include <GameCore/GameGeometry.h>

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

StructuralLineTool::StructuralLineTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : LineTool(
        ToolType::StructuralLine,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

ElectricalLineTool::ElectricalLineTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : LineTool(
        ToolType::ElectricalLine,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

template<LayerType TLayer>
LineTool<TLayer>::LineTool(
    ToolType toolType,
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view)
    , mOriginalLayerClone(modelController.GetModel().CloneLayer<TLayer>())
    , mTempVisualizationDirtyShipRegion()
    , mEngagementData()
{
    wxImage cursorImage = WxHelpers::LoadCursorImage("crosshair_cursor", 15, 15, resourceLocator);
    SetCursor(cursorImage);

    // Check if we need to immediately do a temp visualization
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DoTempVisualization(*mouseCoordinates);

        // Visualize
        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
}

template<LayerType TLayer>
LineTool<TLayer>::~LineTool()
{
    // Mend our temporary visualization, if any
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);

        // Visualize
        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
}

template<LayerType TLayer>
void LineTool<TLayer>::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    // Assuming L/R button transitions already communicated

    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    // Do temp visualization
    DoTempVisualization(mouseCoordinates);

    // Visualize
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnLeftMouseDown()
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Engage
    if (!mEngagementData)
    {
        StartEngagement(mouseCoordinates, MaterialPlaneType::Foreground);

        assert(mEngagementData);
    }

    // Do temp visualization
    DoTempVisualization(mUserInterface.GetMouseCoordinates());

    // Visualize
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnLeftMouseUp()
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Disengage, eventually
    if (mEngagementData)
    {
        EndEngagement(mouseCoordinates);

        assert(!mEngagementData);
    }

    // Do temp visualization
    DoTempVisualization(mouseCoordinates);

    // Visualize
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnRightMouseDown()
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Engage
    if (!mEngagementData)
    {
        StartEngagement(mouseCoordinates, MaterialPlaneType::Background);

        assert(mEngagementData);
    }

    // Do temp visualization
    DoTempVisualization(mUserInterface.GetMouseCoordinates());

    // Visualize
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnRightMouseUp()
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Disengage, eventually
    if (mEngagementData)
    {
        EndEngagement(mouseCoordinates);

        assert(!mEngagementData);
    }

    // Do temp visualization
    DoTempVisualization(mouseCoordinates);

    // Visualize
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void LineTool<TLayer>::StartEngagement(
    ShipSpaceCoordinates const & mouseCoordinates,
    MaterialPlaneType plane)
{
    assert(!mEngagementData);

    mEngagementData.emplace(
        mModelController.GetModel().GetDirtyState(),
        mouseCoordinates,
        plane);
}

template<LayerType TLayer>
void LineTool<TLayer>::EndEngagement(ShipSpaceCoordinates const & mouseCoordinates)
{
    assert(mEngagementData);
    assert(!mTempVisualizationDirtyShipRegion);

    LayerMaterialType const * fillMaterial = GetFillMaterial(mEngagementData->Plane);

    std::optional<ShipSpaceRect> editRect;

    GenerateLinePath(
        mEngagementData->StartCoords,
        mouseCoordinates,
        [&](ShipSpaceCoordinates const & pos)
        {
            // Calc applicable rect intersecting pencil with workspace size
            auto const applicableRect = CalculateApplicableRect(pos);
            if (applicableRect)
            {
                if constexpr (TLayer == LayerType::Structural)
                {
                    mModelController.StructuralRegionFill(
                        *applicableRect,
                        fillMaterial);
                }
                else
                {
                    static_assert(TLayer == LayerType::Electrical);

                    assert(applicableRect->size == ShipSpaceSize(1, 1));
                    if (mModelController.IsElectricalParticleAllowedAt(applicableRect->origin))
                    {
                        mModelController.ElectricalRegionFill(
                            *applicableRect,
                            fillMaterial);
                    }
                }

                if (!editRect)
                {
                    editRect = *applicableRect;
                }
                else
                {
                    editRect->UnionWith(*applicableRect);
                }
            }
        });

    if (editRect)
    {
        //
        // Mark layer as dirty
        //

        SetLayerDirty(TLayer);

        //
        // Create undo action
        //

        auto clippedLayerClone = mOriginalLayerClone.Clone(*editRect);

        PushUndoAction(
            _("Line Tool"),
            clippedLayerClone.Buffer.GetByteSize(),
            mEngagementData->OriginalDirtyState,
            [clippedLayerClone = std::move(clippedLayerClone), origin = editRect->origin](Controller & controller) mutable
            {
                controller.RestoreLayerRegionForUndo(std::move(clippedLayerClone), origin);
            });
    }

    //
    // Reset engagement
    //

    mEngagementData.reset();

    //
    // Re-take original layer clone
    //

    mOriginalLayerClone = mModelController.GetModel().CloneLayer<TLayer>();
}

template<LayerType TLayer>
void LineTool<TLayer>::DoTempVisualization(ShipSpaceCoordinates const & mouseCoordinates)
{
    if (mEngagementData)
    {
        //
        // Temp viz with line + dashed line overlay
        //

        LayerMaterialType const * fillMaterial = GetFillMaterial(mEngagementData->Plane);

        std::optional<ShipSpaceRect> tempVisualizationRect;

        View::OverlayMode overlayMode = View::OverlayMode::Default;

        GenerateLinePath(
            mEngagementData->StartCoords,
            mouseCoordinates,
            [&](ShipSpaceCoordinates const & pos)
            {
                // Calc applicable rect intersecting pencil with workspace size
                auto const applicableRect = CalculateApplicableRect(pos);
                if (applicableRect)
                {
                    if constexpr (TLayer == LayerType::Structural)
                    {
                        mModelController.StructuralRegionFillForEphemeralVisualization(
                            *applicableRect,
                            fillMaterial);
                    }
                    else
                    {
                        static_assert(TLayer == LayerType::Electrical);

                        assert(applicableRect->size == ShipSpaceSize(1, 1));
                        if (mModelController.IsElectricalParticleAllowedAt(applicableRect->origin))
                        {
                            mModelController.ElectricalRegionFillForEphemeralVisualization(
                                *applicableRect,
                                fillMaterial);
                        }
                        else
                        {
                            overlayMode = View::OverlayMode::Error;
                        }
                    }

                    if (!tempVisualizationRect)
                    {
                        tempVisualizationRect = *applicableRect;
                    }
                    else
                    {
                        tempVisualizationRect->UnionWith(*applicableRect);
                    }
                }
            });

        mView.UploadDashedLineOverlay(
            mEngagementData->StartCoords,
            mouseCoordinates,
            overlayMode);

        mTempVisualizationDirtyShipRegion = tempVisualizationRect;
    }
    else
    {
        //
        // Temp viz with block fill + rect overlay 
        //

        // No mouse button information, hence choosing foreground plane arbitrarily
        LayerMaterialType const * fillMaterial = GetFillMaterial(MaterialPlaneType::Foreground);

        std::optional<ShipSpaceRect> const affectedRect = CalculateApplicableRect(mouseCoordinates);

        if (affectedRect.has_value())
        {
            View::OverlayMode overlayMode = View::OverlayMode::Default;

            if constexpr (TLayer == LayerType::Structural)
            {
                mModelController.StructuralRegionFillForEphemeralVisualization(
                    *affectedRect,
                    fillMaterial);
            }
            else
            {
                static_assert(TLayer == LayerType::Electrical);

                assert(affectedRect->size == ShipSpaceSize(1, 1));
                if (!mModelController.IsElectricalParticleAllowedAt(affectedRect->origin))
                {
                    overlayMode = View::OverlayMode::Error;
                }

                mModelController.ElectricalRegionFillForEphemeralVisualization(
                    *affectedRect,
                    fillMaterial);
            }

            mView.UploadRectOverlay(
                *affectedRect,
                overlayMode);

            mTempVisualizationDirtyShipRegion = affectedRect;
        }
    }
}

template<LayerType TLayer>
void LineTool<TLayer>::MendTempVisualization()
{
    assert(mTempVisualizationDirtyShipRegion);

    if constexpr (TLayer == LayerType::Structural)
    {
        mModelController.RestoreStructuralLayerRegionForEphemeralVisualization(
            mOriginalLayerClone,
            *mTempVisualizationDirtyShipRegion,
            mTempVisualizationDirtyShipRegion->origin);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        mModelController.RestoreElectricalLayerRegionForEphemeralVisualization(
            mOriginalLayerClone,
            *mTempVisualizationDirtyShipRegion,
            mTempVisualizationDirtyShipRegion->origin);
    }

    if (mEngagementData.has_value())
    {
        mView.RemoveDashedLineOverlay();
    }
    else
    {
        mView.RemoveRectOverlay();
    }

    mTempVisualizationDirtyShipRegion.reset();
}

template<LayerType TLayer>
std::optional<ShipSpaceRect> LineTool<TLayer>::CalculateApplicableRect(ShipSpaceCoordinates const & coords) const
{
    // Anchor in the middle, and vertically from top

    int const lineSize = GetLineSize();
    int const topLeftLineSize =  (lineSize - 1) / 2;

    ShipSpaceCoordinates const origin = ShipSpaceCoordinates(coords.x, coords.y - (lineSize - 1));

    return ShipSpaceRect(origin - ShipSpaceSize(topLeftLineSize, -topLeftLineSize), { lineSize, lineSize })
        .MakeIntersectionWith({ { 0, 0 }, mModelController.GetModel().GetShipSize() });
}

template<LayerType TLayer>
int LineTool<TLayer>::GetLineSize() const
{
    if constexpr (TLayer == LayerType::Structural)
    {
        return static_cast<int>(mWorkbenchState.GetStructuralLineToolSize());
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        return 1;
    }
}

template<LayerType TLayer>
typename LineTool<TLayer>::LayerMaterialType const * LineTool<TLayer>::GetFillMaterial(MaterialPlaneType plane) const
{
    if constexpr (TLayer == LayerType::Structural)
    {
        if (plane == MaterialPlaneType::Foreground)
        {
            return mWorkbenchState.GetStructuralForegroundMaterial();
        }
        else
        {
            assert(plane == MaterialPlaneType::Background);

            return mWorkbenchState.GetStructuralBackgroundMaterial();
        }

    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        if (plane == MaterialPlaneType::Foreground)
        {
            return mWorkbenchState.GetElectricalForegroundMaterial();
        }
        else
        {
            assert(plane == MaterialPlaneType::Background);

            return mWorkbenchState.GetElectricalBackgroundMaterial();
        }
    }
}

}