/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ToolController.h"

#include <GameCore/Vectors.h>

#include <cassert>

ToolController::ToolController(
    ToolType initialToolType,
    wxWindow * parentWindow,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : mInputState()
    , mCurrentTool(nullptr)
    , mAllTools()
    , mParentWindow(parentWindow)
    , mPanCursor()
    , mGameController(gameController)
    , mSoundController(soundController)
    // Cursor
    , mCurrentToolCursor()
    , mCurrentEffectiveAmbientLightIntensity(0.0f)
{
    //
    // Initialize all tools
    //

    mAllTools.emplace_back(
        std::make_unique<MoveTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<MoveAllTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<SmashTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<SawTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<HeatBlasterTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<FireExtinguisherTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<GrabTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<SwirlTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<PinTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<InjectAirBubblesTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<FloodHoseTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<AntiMatterBombTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<ImpactBombTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<RCBombTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<TimerBombTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<WaveMakerTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<TerrainAdjustTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<ScrubTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<RepairStructureTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<ThanosSnapTool>(
            *this,
            gameController,
            soundController,
            resourceLoader));

    // Prepare own cursor(s)
    mPanCursor = wxCursor(LoadCursorImage("pan_cursor", 15, 15, resourceLoader));

    // Set current tool
    this->SetTool(initialToolType);
}

void ToolController::OnMouseMove(
    int x,
    int y)
{
    // Update input state
    mInputState.PreviousMousePosition = mInputState.MousePosition;
    mInputState.MousePosition = vec2f(x, y);

    // Perform action
    if (mInputState.IsRightMouseDown)
    {
        // Perform our pan tool

        // Pan (opposite direction)
        vec2f screenOffset = mInputState.PreviousMousePosition - mInputState.MousePosition;
        mGameController->PanImmediate(screenOffset);
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

void ToolController::OnEffectiveAmbientLightIntensityUpdated(float effectiveAmbientLightIntensity)
{
    mCurrentEffectiveAmbientLightIntensity = effectiveAmbientLightIntensity;

    InternalSetCurrentToolCursor();
}

void ToolController::SetToolCursor(wxImage const & basisImage, float strength)
{
    mCurrentToolCursor = ToolCursor(basisImage, strength);

    InternalSetCurrentToolCursor();
}

void ToolController::InternalSetCurrentToolCursor()
{
    static int constexpr CursorStep = 30;

    //
    // Process basis image with ambient light blending and power bar
    //

    // Make copy of cursor image
    wxImage newCursorImage = mCurrentToolCursor.BasisImage.Copy();

    int const imageWidth = newCursorImage.GetWidth();
    int const imageHeight = newCursorImage.GetHeight();
    unsigned char * const data = newCursorImage.GetData();

    // Calculate height of power bar
    bool const isMaxStrength = (mCurrentToolCursor.Strength == 1.0f);
    int const powerHeight = static_cast<int>(floorf(static_cast<float>(imageHeight) * mCurrentToolCursor.Strength));

    // Start from top
    for (int y = imageHeight - powerHeight; y < imageHeight; ++y)
    {
        int rowStartIndex = (imageWidth * y);

        // TODO: ambient light blending

        // Red   = 0xDB0F0F
        // Green = 0x039B0A (final)

        float const targetR = (isMaxStrength ? 0x03 : 0xDB) / 255.0f;
        float const targetG = (isMaxStrength ? 0x9B : 0x0F) / 255.0f;
        float const targetB = (isMaxStrength ? 0x0A : 0x0F) / 255.0f;

        for (int x = 0; x < imageWidth; ++x)
        {
            data[(rowStartIndex + x) * 3] = static_cast<unsigned char>(targetR * 255.0f);
            data[(rowStartIndex + x) * 3 + 1] = static_cast<unsigned char>(targetG * 255.0f);
            data[(rowStartIndex + x) * 3 + 2] = static_cast<unsigned char>(targetB * 255.0f);
        }
    }

    mParentWindow->SetCursor(wxCursor(newCursorImage));
}