/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tools.h"

#include "WxHelpers.h"

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
        WxHelpers::LoadCursorImage("move_cursor_up", 13, 5, resourceLoader),
        WxHelpers::LoadCursorImage("move_cursor_down", 13, 5, resourceLoader),
        WxHelpers::LoadCursorImage("move_cursor_rotate_up", 13, 5, resourceLoader),
        WxHelpers::LoadCursorImage("move_cursor_rotate_down", 13, 5, resourceLoader))
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
        WxHelpers::LoadCursorImage("move_all_cursor_up", 13, 5, resourceLoader),
        WxHelpers::LoadCursorImage("move_all_cursor_down", 13, 5, resourceLoader),
        WxHelpers::LoadCursorImage("move_all_cursor_rotate_up", 13, 5, resourceLoader),
        WxHelpers::LoadCursorImage("move_all_cursor_rotate_down", 13, 5, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// Pick and Pull
////////////////////////////////////////////////////////////////////////

PickAndPullTool::PickAndPullTool(
    IToolCursorManager & toolCursorManager,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::PickAndPull,
        toolCursorManager,
        std::move(gameController),
        std::move(soundController))
    , mUpCursorImage(WxHelpers::LoadCursorImage("pliers_cursor_up", 2, 2, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("pliers_cursor_down", 2, 2, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("smash_cursor_up", 6, 9, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("smash_cursor_down", 6, 9, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("chainsaw_cursor_up", 8, 20, resourceLoader))
    , mDownCursorImage1(WxHelpers::LoadCursorImage("chainsaw_cursor_down_1", 8, 20, resourceLoader))
    , mDownCursorImage2(WxHelpers::LoadCursorImage("chainsaw_cursor_down_2", 8, 20, resourceLoader))
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
    , mHeatUpCursorImage(WxHelpers::LoadCursorImage("heat_blaster_heat_cursor_up", 5, 1, resourceLoader))
    , mCoolUpCursorImage(WxHelpers::LoadCursorImage("heat_blaster_cool_cursor_up", 5, 30, resourceLoader))
    , mHeatDownCursorImage(WxHelpers::LoadCursorImage("heat_blaster_heat_cursor_down", 5, 1, resourceLoader))
    , mCoolDownCursorImage(WxHelpers::LoadCursorImage("heat_blaster_cool_cursor_down", 5, 30, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("fire_extinguisher_cursor_up", 6, 3, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("fire_extinguisher_cursor_down", 6, 3, resourceLoader))
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
    , mUpPlusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_up_plus", 15, 15, resourceLoader))
    , mUpMinusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_up_minus", 15, 15, resourceLoader))
    , mDownPlusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_down_plus", 15, 15, resourceLoader))
    , mDownMinusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_down_minus", 15, 15, resourceLoader))
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
    , mUpPlusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_up_cw", 15, 15, resourceLoader))
    , mUpMinusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_up_ccw", 15, 15, resourceLoader))
    , mDownPlusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_down_cw", 15, 15, resourceLoader))
    , mDownMinusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_down_ccw", 15, 15, resourceLoader))
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
    , mCursorImage(WxHelpers::LoadCursorImage("pin_cursor", 4, 27, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("air_tank_cursor_up", 12, 1, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("air_tank_cursor_down", 12, 1, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("flood_cursor_up", 20, 0, resourceLoader))
    , mDownCursorImage1(WxHelpers::LoadCursorImage("flood_cursor_down_1", 20, 0, resourceLoader))
    , mDownCursorImage2(WxHelpers::LoadCursorImage("flood_cursor_down_2", 20, 0, resourceLoader))
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
    , mCursorImage(WxHelpers::LoadCursorImage("am_bomb_cursor", 16, 16, resourceLoader))
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
    , mCursorImage(WxHelpers::LoadCursorImage("impact_bomb_cursor", 18, 10, resourceLoader))
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
    , mCursorImage(WxHelpers::LoadCursorImage("rc_bomb_cursor", 16, 21, resourceLoader))
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
    , mCursorImage(WxHelpers::LoadCursorImage("timer_bomb_cursor", 16, 19, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("wave_maker_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("wave_maker_cursor_down", 15, 15, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_down", 15, 15, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("scrub_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("scrub_cursor_down", 15, 15, resourceLoader))
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("repair_structure_cursor_up", 8, 8, resourceLoader))
    , mDownCursorImages{
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_0", 8, 8, resourceLoader),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_1", 8, 8, resourceLoader),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_2", 8, 8, resourceLoader),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_3", 8, 8, resourceLoader),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_4", 8, 8, resourceLoader) }
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
    , mUpCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_up", 15, 15, resourceLoader))
    , mDownCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_down", 15, 15, resourceLoader))
{
}