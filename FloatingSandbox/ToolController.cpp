/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ToolController.h"

#include <UILib/WxHelpers.h>

#include <GameCore/GameMath.h>
#include <GameCore/Vectors.h>

#include <cmath>

ToolController::ToolController(
    ToolType initialToolType,
    float initialEffectiveAmbientLightIntensity,
    wxWindow * parentWindow,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : mInputState()
    , mCurrentTool(nullptr)
    , mAllTools()
    , mParentWindow(parentWindow)
    , mPanCursor()
    , mGameController(gameController)
    , mSoundController(soundController)
    // Cursor
    , mCurrentToolCursor()
    , mCurrentToolCursorBrightness(CalculateCursorBrightness(initialEffectiveAmbientLightIntensity))
{
    //
    // Initialize all tools
    //

    mAllTools.emplace_back(
        std::make_unique<MoveTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<MoveAllTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<PickAndPullTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<SmashTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<SawTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<HeatBlasterTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<FireExtinguisherTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<GrabTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<SwirlTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<PinTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<InjectPressureTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<FloodHoseTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<AntiMatterBombTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<ImpactBombTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<RCBombTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<TimerBombTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<WaveMakerTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<TerrainAdjustTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<ScrubTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<RepairStructureTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<ThanosSnapTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<ScareFishTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<PhysicsProbeTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<BlastTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<ElectricSparkTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<WindMakerTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<LaserCannonTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<LampTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<PlaceFurnitureNpcTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<PlaceHumanNpcTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<MoveNpcTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<RemoveNpcTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    mAllTools.emplace_back(
        std::make_unique<FollowNpcTool>(
            *this,
            gameController,
            soundController,
            resourceLocator));

    // Prepare own cursor(s)
    mPanCursor = WxHelpers::LoadCursor("pan_cursor", 15, 15, resourceLocator);

    // Set current tool
    this->SetTool(initialToolType);
}

void ToolController::OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition)
{
    // Update input state
    mInputState.PreviousMousePosition = mInputState.MousePosition;
    mInputState.MousePosition = mouseScreenPosition;

    // Perform action
    if (mInputState.IsRightMouseDown)
    {
        // Perform our pan tool

        // Pan (opposite direction)
        DisplayLogicalSize screenOffset = mInputState.PreviousMousePosition - mInputState.MousePosition;
        mGameController.Pan(screenOffset);
    }
    else
    {
        // Perform current tool's action
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->OnMouseMove(mInputState);
        }
    }
}

void ToolController::OnLeftMouseDown()
{
    // Update input state
    mInputState.IsLeftMouseDown = true;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnLeftMouseDown(mInputState);
    }
}

void ToolController::OnLeftMouseUp()
{
    // Update input state
    mInputState.IsLeftMouseDown = false;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnLeftMouseUp(mInputState);
    }
}

void ToolController::OnRightMouseDown()
{
    // Update input state
    mInputState.IsRightMouseDown = true;

    // Show our pan cursor
    mParentWindow->SetCursor(mPanCursor);
}

void ToolController::OnRightMouseUp()
{
    // Update input state
    mInputState.IsRightMouseDown = false;

    if (nullptr != mCurrentTool)
    {
        // Show tool's cursor again, since we moved out of Pan
        InternalSetCurrentToolCursor();
    }
}

void ToolController::OnShiftKeyDown()
{
    // Update input state
    mInputState.IsShiftKeyDown = true;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnShiftKeyDown(mInputState);
    }
}

void ToolController::OnShiftKeyUp()
{
    // Update input state
    mInputState.IsShiftKeyDown = false;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnShiftKeyUp(mInputState);
    }
}

void ToolController::SetToolCursor(wxImage const & basisImage, float strength)
{
    mCurrentToolCursor = ToolCursor(basisImage, strength);

    InternalSetCurrentToolCursor();
}

float ToolController::CalculateCursorBrightness(float effectiveAmbientLightIntensity)
{
    return Step(0.5f, 1.0f - effectiveAmbientLightIntensity);
}

void ToolController::InternalSetCurrentToolCursor()
{
    //
    // Process basis image with ambient light blending and power bar
    //

    // Make copy of cursor image
    wxImage newCursorImage = mCurrentToolCursor.BasisImage.Copy();

    int const imageWidth = newCursorImage.GetWidth();
    int const imageHeight = newCursorImage.GetHeight();
    unsigned char * const data = newCursorImage.GetData();

    // Calculate height of power bar
    int const powerHeight = static_cast<int>(std::floor(static_cast<float>(imageHeight) * mCurrentToolCursor.Strength));

    // Calculate target power bar color
    // Red   = 0xDB0F0F
    // Green = 0x039B0A (final)
    rgbColor const powerColor = (mCurrentToolCursor.Strength == 1.0f)
        ? rgbColor(0x03, 0x9B, 0x0A)
        : rgbColor(0xDB, 0x0F, 0x0F);

    // Target color when the scene is dark
    vec3f constexpr LightenedCursorColor(0.8f, 0.8f, 0.8f);

    // Start from top
    for (int y = 0; y < imageHeight; ++y)
    {
        int rowStartIndex = (imageWidth * y);

        if (y >= imageHeight - powerHeight)
        {
            // Power zone
            for (int x = 0; x < imageWidth; ++x)
            {
                data[(rowStartIndex + x) * 3] = powerColor.r;
                data[(rowStartIndex + x) * 3 + 1] = powerColor.g;
                data[(rowStartIndex + x) * 3 + 2] = powerColor.b;
            }
        }
        else
        {
            // Original color, adjusted for ambient light
            for (int x = 0; x < imageWidth; ++x)
            {
                vec3f originalColor = rgbColor(
                    data[(rowStartIndex + x) * 3],
                    data[(rowStartIndex + x) * 3 + 1],
                    data[(rowStartIndex + x) * 3 + 2]).toVec3f();

                // Linear interpolation
                vec3f targetColor = Mix(
                    originalColor,
                    LightenedCursorColor,
                    mCurrentToolCursorBrightness);

                rgbColor targetRgbColor = rgbColor(targetColor);

                data[(rowStartIndex + x) * 3] = targetRgbColor.r;
                data[(rowStartIndex + x) * 3 + 1] = targetRgbColor.g;
                data[(rowStartIndex + x) * 3 + 2] = targetRgbColor.b;
            }
        }
    }

    mParentWindow->SetCursor(wxCursor(newCursorImage));
}