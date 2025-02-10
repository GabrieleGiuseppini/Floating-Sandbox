/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureEraserTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

#include <Core/GameGeometry.h>

#include <type_traits>

namespace ShipBuilder {

ExteriorTextureEraserTool::ExteriorTextureEraserTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : TextureEraserTool(
        ToolType::ExteriorTextureEraser,
        controller,
        gameAssetManager)
{}

InteriorTextureEraserTool::InteriorTextureEraserTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : TextureEraserTool(
        ToolType::InteriorTextureEraser,
        controller,
        gameAssetManager)
{}

template<LayerType TLayerType>
TextureEraserTool<TLayerType>::TextureEraserTool(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        controller)
    , mOriginalLayerClone(mController.GetModelController().CloneExistingLayer<TLayerType>())
    , mTempVisualizationDirtyTextureRegion()
    , mEngagementData()
    , mIsShiftDown(false)
{
    SetCursor(WxHelpers::LoadCursorImage("eraser_cursor", 8, 27, gameAssetManager));

    //
    // Do initial visualization
    //

    // Calculate affected rect
    std::optional<ImageRect> const affectedRect = CalculateApplicableRect(ScreenToTextureSpace(TLayerType, GetCurrentMouseCoordinates()));

    // Apply (temporary) change
    if (affectedRect)
    {
        DoTempVisualization(*affectedRect);

        assert(mTempVisualizationDirtyTextureRegion);

        mController.LayerChangeEpilog();
    }
}

template<LayerType TLayerType>
TextureEraserTool<TLayerType>::~TextureEraserTool()
{
    Leave();
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    // Assuming L/R button transitions already communicated

    auto const mouseCoordinatesInTextureSpace = ScreenToTextureSpace(TLayerType, mouseCoordinates);

    if (!mEngagementData)
    {
        //
        // Temp visualization
        //

        // Calculate affected rect
        std::optional<ImageRect> const affectedRect = CalculateApplicableRect(mouseCoordinatesInTextureSpace);

        if (affectedRect != mTempVisualizationDirtyTextureRegion)
        {
            // Restore previous temp visualization
            if (mTempVisualizationDirtyTextureRegion)
            {
                MendTempVisualization();

                assert(!mTempVisualizationDirtyTextureRegion);
            }

            // Apply (temporary) change
            if (affectedRect)
            {
                DoTempVisualization(*affectedRect);

                assert(mTempVisualizationDirtyTextureRegion);
            }

            mController.LayerChangeEpilog();
        }
    }
    else
    {
        DoEdit(mouseCoordinatesInTextureSpace);
    }
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::OnLeftMouseDown()
{
    auto const mouseCoordinatesInTextureSpace = ScreenToTextureSpace(TLayerType, GetCurrentMouseCoordinates());

    // Restore temp visualization
    if (mTempVisualizationDirtyTextureRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyTextureRegion);
    }

    if (!mEngagementData)
    {
        StartEngagement(mouseCoordinatesInTextureSpace);

        assert(mEngagementData);
    }

    DoEdit(mouseCoordinatesInTextureSpace);
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        EndEngagement();

        assert(!mEngagementData);
    }

    // Note: we don't start temp visualization, as the current mouse position
    // already has the edit (as permanent)
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        // Remember initial engagement
        assert(!mEngagementData->ShiftLockInitialPosition.has_value());
        mEngagementData->ShiftLockInitialPosition = ScreenToTextureSpace(TLayerType, GetCurrentMouseCoordinates());
    }
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        // Forget engagement
        assert(mEngagementData->ShiftLockInitialPosition.has_value());
        mEngagementData->ShiftLockInitialPosition.reset();
        mEngagementData->ShiftLockIsVertical.reset();
    }
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::OnMouseLeft()
{
    Leave();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::Leave()
{
    // Mend our temporary visualization, if any
    if (mTempVisualizationDirtyTextureRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyTextureRegion);

        mController.LayerChangeEpilog();
    }
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::StartEngagement(ImageCoordinates const & mouseCoordinates)
{
    assert(!mEngagementData);

    mEngagementData.emplace(
        mController.GetModelController().GetDirtyState(),
        mIsShiftDown ? mouseCoordinates : std::optional<ImageCoordinates>());
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::DoEdit(ImageCoordinates const & mouseCoordinates)
{
    assert(mEngagementData);

    bool hasEdited = false;

    // Calculate SHIFT lock direction, if needed
    if (mEngagementData->ShiftLockInitialPosition.has_value() && !mEngagementData->ShiftLockIsVertical.has_value())
    {
        if (mouseCoordinates != *(mEngagementData->ShiftLockInitialPosition))
        {
            // Constrain to either horizontally or vertically, wrt SHIFT lock's initial position
            if (std::abs(mouseCoordinates.x - mEngagementData->ShiftLockInitialPosition->x) > std::abs(mouseCoordinates.y - mEngagementData->ShiftLockInitialPosition->y))
            {
                // X is larger
                mEngagementData->ShiftLockIsVertical = false;
            }
            else
            {
                // Y is larger
                mEngagementData->ShiftLockIsVertical = true;
            }
        }
    }

    // Calculate actual mouse coordinates - adjusted for SHIFT lock
    ImageCoordinates actualMouseCoordinates = mouseCoordinates;
    if (mEngagementData->ShiftLockIsVertical.has_value())
    {
        assert(mEngagementData->ShiftLockInitialPosition.has_value());

        if (*(mEngagementData->ShiftLockIsVertical))
        {
            actualMouseCoordinates.x = mEngagementData->ShiftLockInitialPosition->x;
        }
        else
        {
            actualMouseCoordinates.y = mEngagementData->ShiftLockInitialPosition->y;
        }
    }

    // Calculate start point
    ImageCoordinates const startPoint = (mEngagementData->PreviousEngagementPosition.has_value())
        ? *mEngagementData->PreviousEngagementPosition
        : actualMouseCoordinates;

    // Calculate end point
    ImageCoordinates const endPoint = actualMouseCoordinates;

    // Generate line
    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        startPoint,
        endPoint,
        [&](ImageCoordinates const & pos)
        {
            // Calc applicable rect intersecting pencil with workspace size
            auto const applicableRect = CalculateApplicableRect(pos);
            if (applicableRect)
            {
                if constexpr (TLayerType == LayerType::ExteriorTexture)
                {
                    mController.GetModelController().EraseExteriorTextureRegion(*applicableRect);
                }
                else
                {
                    static_assert(TLayerType == LayerType::InteriorTexture);

                    mController.GetModelController().EraseInteriorTextureRegion(*applicableRect);
                }

                // Update edit region
                if (!mEngagementData->EditRegion)
                {
                    mEngagementData->EditRegion = *applicableRect;
                }
                else
                {
                    mEngagementData->EditRegion->UnionWith(*applicableRect);
                }

                hasEdited = true;
            }
        });

    // Update previous engagement
    mEngagementData->PreviousEngagementPosition = endPoint;

    // Epilog
    mController.LayerChangeEpilog({ TLayerType });
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::EndEngagement()
{
    assert(mEngagementData);

    if (mEngagementData->EditRegion)
    {
        //
        // Create undo action
        //

        auto clippedLayerBackup = mOriginalLayerClone.MakeRegionBackup(*mEngagementData->EditRegion);
        auto const clipByteSize = clippedLayerBackup.Buffer.GetByteSize();

        if constexpr (TLayerType == LayerType::ExteriorTexture)
        {
            mController.StoreUndoAction(
                _("Eraser Exterior"),
                clipByteSize,
                mEngagementData->OriginalDirtyState,
                [clippedLayerBackup = std::move(clippedLayerBackup), origin = mEngagementData->EditRegion->origin](Controller & controller) mutable
                {
                    controller.RestoreExteriorTextureLayerRegionBackupForUndo(std::move(clippedLayerBackup), origin);
                });
        }
        else
        {
            static_assert(TLayerType == LayerType::InteriorTexture);

            mController.StoreUndoAction(
                _("Eraser Interior"),
                clipByteSize,
                mEngagementData->OriginalDirtyState,
                [clippedLayerBackup = std::move(clippedLayerBackup), origin = mEngagementData->EditRegion->origin](Controller & controller) mutable
                {
                    controller.RestoreInteriorTextureLayerRegionBackupForUndo(std::move(clippedLayerBackup), origin);
                });
        }
    }

    //
    // Reset engagement
    //

    mEngagementData.reset();

    //
    // Restart temp visualization
    //

    assert(!mTempVisualizationDirtyTextureRegion);

    // Re-take original layer clone
    mOriginalLayerClone = mController.GetModelController().CloneExistingLayer<TLayerType>();
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::DoTempVisualization(ImageRect const & affectedRect)
{
    if constexpr (TLayerType == LayerType::ExteriorTexture)
    {
        mController.GetModelController().ExteriorTextureRegionEraseForEphemeralVisualization(affectedRect);

        mController.GetView().UploadRectOverlayExteriorTexture(
            affectedRect,
            View::OverlayMode::Default);
    }
    else
    {
        static_assert(TLayerType == LayerType::InteriorTexture);

        mController.GetModelController().InteriorTextureRegionEraseForEphemeralVisualization(affectedRect);

        mController.GetView().UploadRectOverlayInteriorTexture(
            affectedRect,
            View::OverlayMode::Default);
    }

    mTempVisualizationDirtyTextureRegion = affectedRect;
}

template<LayerType TLayerType>
void TextureEraserTool<TLayerType>::MendTempVisualization()
{
    assert(mTempVisualizationDirtyTextureRegion);

    if constexpr (TLayerType == LayerType::ExteriorTexture)
    {
        mController.GetModelController().RestoreExteriorTextureLayerRegionEphemeralVisualization(
            mOriginalLayerClone.Buffer,
            *mTempVisualizationDirtyTextureRegion,
            mTempVisualizationDirtyTextureRegion->origin);
    }
    else
    {
        static_assert(TLayerType == LayerType::InteriorTexture);

        mController.GetModelController().RestoreInteriorTextureLayerRegionEphemeralVisualization(
            mOriginalLayerClone.Buffer,
            *mTempVisualizationDirtyTextureRegion,
            mTempVisualizationDirtyTextureRegion->origin);
    }

    mController.GetView().RemoveRectOverlay();

    mTempVisualizationDirtyTextureRegion.reset();
}

template<LayerType TLayerType>
std::optional<ImageRect> TextureEraserTool<TLayerType>::CalculateApplicableRect(ImageCoordinates const & coords) const
{
    // Anchor in the middle, and vertically from top

    int const pencilSize = mController.GetWorkbenchState().GetTextureEraserToolSize();
    int const topLeftPencilSize =  (pencilSize - 1) / 2;

    ImageCoordinates const origin = ImageCoordinates(coords.x, coords.y - (pencilSize - 1));

    ImageRect  textureRect;
    if constexpr (TLayerType == LayerType::ExteriorTexture)
    {
        textureRect = { { 0, 0}, mController.GetModelController().GetExteriorTextureSize() };
    }
    else
    {
        static_assert(TLayerType == LayerType::InteriorTexture);

        textureRect = { { 0, 0}, mController.GetModelController().GetInteriorTextureSize() };
    }

    return ImageRect(origin - ImageSize(topLeftPencilSize, -topLeftPencilSize), ImageSize(pencilSize, pencilSize))
        .MakeIntersectionWith(textureRect);
}

}