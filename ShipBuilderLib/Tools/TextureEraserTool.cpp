/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureEraserTool.h"

#include "Controller.h"

#include <GameCore/GameGeometry.h>

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

TextureEraserTool::TextureEraserTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::TextureEraser,
        controller)
    , mOriginalLayerClone(mController.GetModelController().CloneExistingLayer<LayerType::Texture>())
    , mTempVisualizationDirtyTextureRegion()
    , mEngagementData()
    , mIsShiftDown(false)
{
    SetCursor(WxHelpers::LoadCursorImage("eraser_cursor", 8, 27, resourceLocator));

    //
    // Do initial visualization
    //

    // Calculate affected rect
    std::optional<ImageRect> const affectedRect = CalculateApplicableRect(ScreenToTextureSpace(GetCurrentMouseCoordinates()));

    // Apply (temporary) change
    if (affectedRect)
    {
        DoTempVisualization(*affectedRect);

        assert(mTempVisualizationDirtyTextureRegion);

        mController.LayerChangeEpilog();
    }
}

TextureEraserTool::~TextureEraserTool()
{
    Leave();
}

void TextureEraserTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    // Assuming L/R button transitions already communicated

    auto const mouseCoordinatesInTextureSpace = ScreenToTextureSpace(mouseCoordinates);

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

void TextureEraserTool::OnLeftMouseDown()
{
    auto const mouseCoordinatesInTextureSpace = ScreenToTextureSpace(GetCurrentMouseCoordinates());

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

void TextureEraserTool::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        EndEngagement();

        assert(!mEngagementData);
    }

    // Note: we don't start temp visualization, as the current mouse position
    // already has the edit (as permanent)
}

void TextureEraserTool::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        // Remember initial engagement
        assert(!mEngagementData->ShiftLockInitialPosition.has_value());
        mEngagementData->ShiftLockInitialPosition = ScreenToTextureSpace(GetCurrentMouseCoordinates());
    }
}

void TextureEraserTool::OnShiftKeyUp()
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

void TextureEraserTool::OnMouseLeft()
{
    Leave();
}

//////////////////////////////////////////////////////////////////////////////

void TextureEraserTool::Leave()
{
    // Mend our temporary visualization, if any
    if (mTempVisualizationDirtyTextureRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyTextureRegion);

        mController.LayerChangeEpilog();
    }
}

void TextureEraserTool::StartEngagement(ImageCoordinates const & mouseCoordinates)
{
    assert(!mEngagementData);

    mEngagementData.emplace(
        mController.GetModelController().GetDirtyState(),
        mIsShiftDown ? mouseCoordinates : std::optional<ImageCoordinates>());
}

void TextureEraserTool::DoEdit(ImageCoordinates const & mouseCoordinates)
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
    GenerateIntegralLinePath<IntegralLineType::Minimal>(
        startPoint,
        endPoint,
        [&](ImageCoordinates const & pos)
        {
            // Calc applicable rect intersecting pencil with workspace size
            auto const applicableRect = CalculateApplicableRect(pos);
            if (applicableRect)
            {
                mController.GetModelController().TextureRegionErase(*applicableRect);
                
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
    mController.LayerChangeEpilog(LayerType::Texture);
}

void TextureEraserTool::EndEngagement()
{
    assert(mEngagementData);

    if (mEngagementData->EditRegion)
    {
        //
        // Create undo action
        //

        auto clippedLayerBackup = mOriginalLayerClone.MakeRegionBackup(*mEngagementData->EditRegion);
        auto const clipByteSize = clippedLayerBackup.Buffer.GetByteSize();

        mController.StoreUndoAction(
            _("Eraser Texture"),
            clipByteSize,
            mEngagementData->OriginalDirtyState,
            [clippedLayerBackup = std::move(clippedLayerBackup), origin = mEngagementData->EditRegion->origin](Controller & controller) mutable
            {
                controller.RestoreTextureLayerRegionBackupForUndo(std::move(clippedLayerBackup), origin);
            });
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
    mOriginalLayerClone = mController.GetModelController().CloneExistingLayer<LayerType::Texture>();
}

void TextureEraserTool::DoTempVisualization(ImageRect const & affectedRect)
{
    mController.GetModelController().TextureRegionEraseForEphemeralVisualization(affectedRect);

    mController.GetView().UploadRectOverlay(
        affectedRect,
        View::OverlayMode::Default);

    mTempVisualizationDirtyTextureRegion = affectedRect;
}

void TextureEraserTool::MendTempVisualization()
{
    assert(mTempVisualizationDirtyTextureRegion);

    mController.GetModelController().RestoreTextureLayerRegionForEphemeralVisualization(
        mOriginalLayerClone,
        *mTempVisualizationDirtyTextureRegion,
        mTempVisualizationDirtyTextureRegion->origin);

    mController.GetView().RemoveRectOverlay();

    mTempVisualizationDirtyTextureRegion.reset();
}

std::optional<ImageRect> TextureEraserTool::CalculateApplicableRect(ImageCoordinates const & coords) const
{
    // Anchor in the middle, and vertically from top

    int const pencilSize = mController.GetWorkbenchState().GetTextureEraserToolSize();
    int const topLeftPencilSize =  (pencilSize - 1) / 2;

    ImageCoordinates const origin = ImageCoordinates(coords.x, coords.y - (pencilSize - 1));

    return ImageRect(origin - ImageSize(topLeftPencilSize, -topLeftPencilSize), ImageSize(pencilSize, pencilSize))
        .MakeIntersectionWith({ { 0, 0 }, mController.GetModelController().GetTextureSize()});
}

}