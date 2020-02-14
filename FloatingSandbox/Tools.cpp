/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tools.h"

#include <GameCore/GameException.h>

#include <wx/bitmap.h>
#include <wx/image.h>

#include <cassert>

wxImage LoadCursorImage(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader)
{
    auto filepath = resourceLoader.GetCursorFilepath(cursorName);
    auto bmp = std::make_unique<wxBitmap>(filepath.string(), wxBITMAP_TYPE_PNG);

    wxImage img = bmp->ConvertToImage();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    return img;
}

/////////////////////////////////////////////////////////////////////////
// One-Shot Tool
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// Continuous Tool
/////////////////////////////////////////////////////////////////////////

void ContinuousTool::Update(InputState const & inputState)
{
    // We apply the tool only if the left mouse button is down
    if (inputState.IsLeftMouseDown)
    {
        auto const now = std::chrono::steady_clock::now();

        // Accumulate total time iff we haven't moved since last time
        if (mPreviousMousePosition == inputState.MousePosition)
        {
            mCumulatedTime += std::chrono::duration_cast<std::chrono::microseconds>(now - mPreviousTimestamp);
        }
        else
        {
            // We've moved, don't accumulate but use time
            // that was built up so far
        }

        // Remember new position and timestamp
        mPreviousMousePosition = inputState.MousePosition;
        mPreviousTimestamp = now;

        // Apply current tool
        ApplyTool(
            mCumulatedTime,
            inputState);
    }
}

////////////////////////////////////////////////////////////////////////
// Move
////////////////////////////////////////////////////////////////////////

MoveTool::MoveTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : BaseMoveTool(
        ToolType::Move,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController),
        LoadCursorImage("move_cursor_up", 13, 5, resourceLoader),
        LoadCursorImage("move_cursor_down", 13, 5, resourceLoader),
        LoadCursorImage("move_cursor_rotate_up", 13, 5, resourceLoader),
        LoadCursorImage("move_cursor_rotate_down", 13, 5, resourceLoader))
{
}

MoveAllTool::MoveAllTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : BaseMoveTool(
        ToolType::MoveAll,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController),
        LoadCursorImage("move_all_cursor_up", 13, 5, resourceLoader),
        LoadCursorImage("move_all_cursor_down", 13, 5, resourceLoader),
        LoadCursorImage("move_all_cursor_rotate_up", 13, 5, resourceLoader),
        LoadCursorImage("move_all_cursor_rotate_down", 13, 5, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// Smash
////////////////////////////////////////////////////////////////////////

SmashTool::SmashTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Smash,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpCursorImage(LoadCursorImage("smash_cursor_up", 6, 9, resourceLoader))
    , mDownCursorImage(LoadCursorImage("smash_cursor_down", 6, 9, resourceLoader))
{
}

void SmashTool::ApplyTool(
    std::chrono::microseconds const & cumulatedTime,
    InputState const & inputState)
{
    // Calculate radius fraction
    // 0-500ms      = 0.1
    // ...
    // 5000ms-+INF = 1.0

    static constexpr float MinFraction = 0.1f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float radiusFraction = MinFraction + (1.0f - MinFraction) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    mToolCursorManager.SetToolCursor(mDownCursorImage, radiusFraction);

    // Destroy
    mGameController->DestroyAt(
        inputState.MousePosition,
        radiusFraction);
}

////////////////////////////////////////////////////////////////////////
// Saw
////////////////////////////////////////////////////////////////////////

SawTool::SawTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::Saw,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpCursorImage(LoadCursorImage("chainsaw_cursor_up", 8, 20, resourceLoader))
    , mDownCursorImage1(LoadCursorImage("chainsaw_cursor_down_1", 8, 20, resourceLoader))
    , mDownCursorImage2(LoadCursorImage("chainsaw_cursor_down_2", 8, 20, resourceLoader))
    , mPreviousMousePos()
    , mDownCursorCounter(0)
    , mIsUnderwater(false)
{
}

////////////////////////////////////////////////////////////////////////
// HeatBlaster
////////////////////////////////////////////////////////////////////////

HeatBlasterTool::HeatBlasterTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::HeatBlaster,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mIsEngaged(false)
    , mCurrentAction(HeatBlasterActionType::Heat)
    , mHeatUpCursorImage(LoadCursorImage("heat_blaster_heat_cursor_up", 5, 1, resourceLoader))
    , mCoolUpCursorImage(LoadCursorImage("heat_blaster_cool_cursor_up", 5, 30, resourceLoader))
    , mHeatDownCursorImage(LoadCursorImage("heat_blaster_heat_cursor_down", 5, 1, resourceLoader))
    , mCoolDownCursorImage(LoadCursorImage("heat_blaster_cool_cursor_down", 5, 30, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// FireExtinguisher
////////////////////////////////////////////////////////////////////////

FireExtinguisherTool::FireExtinguisherTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::FireExtinguisher,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mIsEngaged(false)
    , mUpCursorImage(LoadCursorImage("fire_extinguisher_cursor_up", 6, 3, resourceLoader))
    , mDownCursorImage(LoadCursorImage("fire_extinguisher_cursor_down", 6, 3, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// Grab
////////////////////////////////////////////////////////////////////////

GrabTool::GrabTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Grab,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpPlusCursorImage(LoadCursorImage("drag_cursor_up_plus", 15, 15, resourceLoader))
    , mUpMinusCursorImage(LoadCursorImage("drag_cursor_up_minus", 15, 15, resourceLoader))
    , mDownPlusCursorImage(LoadCursorImage("drag_cursor_down_plus", 15, 15, resourceLoader))
    , mDownMinusCursorImage(LoadCursorImage("drag_cursor_down_minus", 15, 15, resourceLoader))
{
}

void GrabTool::ApplyTool(
    std::chrono::microseconds const & cumulatedTime,
    InputState const & inputState)
{
    // Calculate strength multiplier
    // 0-500ms      = 0.1
    // ...
    // 5000ms-+INF = 1.0

    static constexpr float MinFraction = 0.1f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float strengthFraction = MinFraction + (1.0f - MinFraction) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    mToolCursorManager.SetToolCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursorImage : mDownPlusCursorImage,
        strengthFraction);

    // Draw
    mGameController->DrawTo(
        inputState.MousePosition,
        inputState.IsShiftKeyDown
            ? -strengthFraction
            : strengthFraction);
}

////////////////////////////////////////////////////////////////////////
// Swirl
////////////////////////////////////////////////////////////////////////

SwirlTool::SwirlTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Grab,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpPlusCursorImage(LoadCursorImage("swirl_cursor_up_cw", 15, 15, resourceLoader))
    , mUpMinusCursorImage(LoadCursorImage("swirl_cursor_up_ccw", 15, 15, resourceLoader))
    , mDownPlusCursorImage(LoadCursorImage("swirl_cursor_down_cw", 15, 15, resourceLoader))
    , mDownMinusCursorImage(LoadCursorImage("swirl_cursor_down_ccw", 15, 15, resourceLoader))
{
}

void SwirlTool::ApplyTool(
    std::chrono::microseconds const & cumulatedTime,
    InputState const & inputState)
{
    // Calculate strength multiplier
    // 0-500ms      = 0.1
    // ...
    // 5000ms-+INF = 1.0

    static constexpr float MinFraction = 0.1f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float strengthFraction = MinFraction + (1.0f - MinFraction) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    mToolCursorManager.SetToolCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursorImage : mDownPlusCursorImage,
        strengthFraction);

    // Draw
    mGameController->SwirlAt(
        inputState.MousePosition,
        inputState.IsShiftKeyDown
            ? -strengthFraction
            : strengthFraction);
}

////////////////////////////////////////////////////////////////////////
// Pin
////////////////////////////////////////////////////////////////////////

PinTool::PinTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::Pin,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mCursorImage(LoadCursorImage("pin_cursor", 4, 27, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// InjectAirBubbles
////////////////////////////////////////////////////////////////////////

InjectAirBubblesTool::InjectAirBubblesTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::InjectAirBubbles,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mIsEngaged(false)
    , mUpCursorImage(LoadCursorImage("air_tank_cursor_up", 12, 1, resourceLoader))
    , mDownCursorImage(LoadCursorImage("air_tank_cursor_down", 12, 1, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// FloodHose
////////////////////////////////////////////////////////////////////////

FloodHoseTool::FloodHoseTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::FloodHose,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mIsEngaged(false)
    , mUpCursorImage(LoadCursorImage("flood_cursor_up", 20, 0, resourceLoader))
    , mDownCursorImage1(LoadCursorImage("flood_cursor_down_1", 20, 0, resourceLoader))
    , mDownCursorImage2(LoadCursorImage("flood_cursor_down_2", 20, 0, resourceLoader))
    , mDownCursorCounter(0)
{
}

////////////////////////////////////////////////////////////////////////
// AntiMatterBomb
////////////////////////////////////////////////////////////////////////

AntiMatterBombTool::AntiMatterBombTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::AntiMatterBomb,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mCursorImage(LoadCursorImage("am_bomb_cursor", 16, 16, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// ImpactBomb
////////////////////////////////////////////////////////////////////////

ImpactBombTool::ImpactBombTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::ImpactBomb,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mCursorImage(LoadCursorImage("impact_bomb_cursor", 18, 10, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// RCBomb
////////////////////////////////////////////////////////////////////////

RCBombTool::RCBombTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::RCBomb,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mCursorImage(LoadCursorImage("rc_bomb_cursor", 16, 21, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// TimerBomb
////////////////////////////////////////////////////////////////////////

TimerBombTool::TimerBombTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::TimerBomb,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mCursorImage(LoadCursorImage("timer_bomb_cursor", 16, 19, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// WaveMaker
////////////////////////////////////////////////////////////////////////

WaveMakerTool::WaveMakerTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::WaveMaker,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpCursorImage(LoadCursorImage("wave_maker_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(LoadCursorImage("wave_maker_cursor_down", 15, 15, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// TerrainAdjust
////////////////////////////////////////////////////////////////////////

TerrainAdjustTool::TerrainAdjustTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::TerrainAdjust,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mCurrentTrajectoryPreviousPosition()
    , mUpCursorImage(LoadCursorImage("terrain_adjust_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(LoadCursorImage("terrain_adjust_cursor_down", 15, 15, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// Scrub
////////////////////////////////////////////////////////////////////////

ScrubTool::ScrubTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::Scrub,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpCursorImage(LoadCursorImage("scrub_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(LoadCursorImage("scrub_cursor_down", 15, 15, resourceLoader))
    , mPreviousMousePos()
    , mPreviousScrub()
    , mPreviousScrubTimestamp(std::chrono::steady_clock::time_point::min())
{
}

////////////////////////////////////////////////////////////////////////
// Repair Structure
////////////////////////////////////////////////////////////////////////

RepairStructureTool::RepairStructureTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::RepairStructure,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mEngagementStartTimestamp()
    , mCurrentSessionId(0)
    , mCurrentSessionStepId(0)
    , mUpCursorImage(LoadCursorImage("repair_structure_cursor_up", 8, 8, resourceLoader))
    , mDownCursorImages{
        LoadCursorImage("repair_structure_cursor_down_0", 8, 8, resourceLoader),
        LoadCursorImage("repair_structure_cursor_down_1", 8, 8, resourceLoader),
        LoadCursorImage("repair_structure_cursor_down_2", 8, 8, resourceLoader),
        LoadCursorImage("repair_structure_cursor_down_3", 8, 8, resourceLoader),
        LoadCursorImage("repair_structure_cursor_down_4", 8, 8, resourceLoader) }
{
}

////////////////////////////////////////////////////////////////////////
// ThanosSnap
////////////////////////////////////////////////////////////////////////

ThanosSnapTool::ThanosSnapTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::ThanosSnap,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpCursorImage(LoadCursorImage("thanos_snap_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(LoadCursorImage("thanos_snap_cursor_down", 15, 15, resourceLoader))
{
}