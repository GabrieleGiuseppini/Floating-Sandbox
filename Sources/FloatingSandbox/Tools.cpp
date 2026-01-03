/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tools.h"

#include <UILib/WxHelpers.h>

/////////////////////////////////////////////////////////////////////////
// One-Shot Tool
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// Continuous Tool
/////////////////////////////////////////////////////////////////////////

void ContinuousTool::UpdateSimulation(InputState const & inputState, float /*currentSimulationTime*/)
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
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : BaseMoveTool(
        ToolType::Move,
        toolCursorManager,
        gameController,
        soundController,
        WxHelpers::LoadCursorImage("move_cursor_up", 13, 5, gameAssetManager),
        WxHelpers::LoadCursorImage("move_cursor_down", 13, 5, gameAssetManager),
        WxHelpers::LoadCursorImage("move_cursor_rotate_up", 13, 5, gameAssetManager),
        WxHelpers::LoadCursorImage("move_cursor_rotate_down", 13, 5, gameAssetManager))
{
}

MoveAllTool::MoveAllTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : BaseMoveTool(
        ToolType::MoveAll,
        toolCursorManager,
        gameController,
        soundController,
        WxHelpers::LoadCursorImage("move_all_cursor_up", 13, 5, gameAssetManager),
        WxHelpers::LoadCursorImage("move_all_cursor_down", 13, 5, gameAssetManager),
        WxHelpers::LoadCursorImage("move_all_cursor_rotate_up", 13, 5, gameAssetManager),
        WxHelpers::LoadCursorImage("move_all_cursor_rotate_down", 13, 5, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// Move Gripped
////////////////////////////////////////////////////////////////////////

MoveGrippedTool::MoveGrippedTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::MoveGripped,
        toolCursorManager,
        gameController,
        soundController)
    , mMoveUpCursorImage(WxHelpers::LoadCursorImage("move_gripped_cursor_up", 11, 20, gameAssetManager))
    , mMoveDownCursorImage(WxHelpers::LoadCursorImage("move_gripped_cursor_down", 11, 20, gameAssetManager))
    , mRotateUpCursorImage(WxHelpers::LoadCursorImage("move_cursor_rotate_up", 13, 5, gameAssetManager))
    , mRotateDownCursorImage(WxHelpers::LoadCursorImage("move_cursor_rotate_down", 13, 5, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// Pick and Pull
////////////////////////////////////////////////////////////////////////

PickAndPullTool::PickAndPullTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::PickAndPull,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("pliers_cursor_up", 2, 2, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("pliers_cursor_down", 2, 2, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// Smash
////////////////////////////////////////////////////////////////////////

SmashTool::SmashTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : ContinuousTool(
        ToolType::Smash,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("smash_cursor_up", 6, 9, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("smash_cursor_down", 6, 9, gameAssetManager))
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

    float const millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());

    float const radiusFraction = MinFraction + (1.0f - MinFraction) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    InternalSetToolCursor(mDownCursorImage, radiusFraction);

    // Destroy
    mGameController.DestroyAt(
        inputState.MousePosition,
        radiusFraction * 10.0f,
        mCurrentSession);
}

////////////////////////////////////////////////////////////////////////
// Saw
////////////////////////////////////////////////////////////////////////

SawTool::SawTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::Saw,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("chainsaw_cursor_up", 8, 20, gameAssetManager))
    , mDownCursorImage1(WxHelpers::LoadCursorImage("chainsaw_cursor_down_1", 8, 20, gameAssetManager))
    , mDownCursorImage2(WxHelpers::LoadCursorImage("chainsaw_cursor_down_2", 8, 20, gameAssetManager))
    , mPreviousMousePos()
    , mCurrentLockedDirection()
    , mIsFirstSegment(false)
    , mDownCursorCounter(0)
    , mIsUnderwater(false)
{
}

////////////////////////////////////////////////////////////////////////
// HeatBlaster
////////////////////////////////////////////////////////////////////////

HeatBlasterTool::HeatBlasterTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::HeatBlaster,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mCurrentAction(HeatBlasterActionType::Heat)
    , mHeatUpCursorImage(WxHelpers::LoadCursorImage("heat_blaster_heat_cursor_up", 5, 1, gameAssetManager))
    , mCoolUpCursorImage(WxHelpers::LoadCursorImage("heat_blaster_cool_cursor_up", 5, 30, gameAssetManager))
    , mHeatDownCursorImage(WxHelpers::LoadCursorImage("heat_blaster_heat_cursor_down", 5, 1, gameAssetManager))
    , mCoolDownCursorImage(WxHelpers::LoadCursorImage("heat_blaster_cool_cursor_down", 5, 30, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// FireExtinguisher
////////////////////////////////////////////////////////////////////////

FireExtinguisherTool::FireExtinguisherTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::FireExtinguisher,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mUpCursorImage(WxHelpers::LoadCursorImage("fire_extinguisher_cursor_up", 6, 3, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("fire_extinguisher_cursor_down", 6, 3, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// Grab
////////////////////////////////////////////////////////////////////////

GrabTool::GrabTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : ContinuousTool(
        ToolType::Grab,
        toolCursorManager,
        gameController,
        soundController)
    , mUpPlusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_up_plus", 15, 15, gameAssetManager))
    , mUpMinusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_up_minus", 15, 15, gameAssetManager))
    , mDownPlusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_down_plus", 15, 15, gameAssetManager))
    , mDownMinusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_down_minus", 15, 15, gameAssetManager))
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
    InternalSetToolCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursorImage : mDownPlusCursorImage,
        strengthFraction);

    // Draw
    mGameController.DrawTo(
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
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : ContinuousTool(
        ToolType::Grab,
        toolCursorManager,
        gameController,
        soundController)
    , mUpPlusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_up_cw", 15, 15, gameAssetManager))
    , mUpMinusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_up_ccw", 15, 15, gameAssetManager))
    , mDownPlusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_down_cw", 15, 15, gameAssetManager))
    , mDownMinusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_down_ccw", 15, 15, gameAssetManager))
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
    InternalSetToolCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursorImage : mDownPlusCursorImage,
        strengthFraction);

    // Swirl
    mGameController.SwirlAt(
        inputState.MousePosition,
        inputState.IsShiftKeyDown
        ? -strengthFraction
        : strengthFraction);
}

////////////////////////////////////////////////////////////////////////
// Anti-Gravity Field
////////////////////////////////////////////////////////////////////////

AntiGravityFieldTool::AntiGravityFieldTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::AntiGravityField,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("anti_gravity_field_cursor", 15, 15, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// Pin
////////////////////////////////////////////////////////////////////////

PinTool::PinTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::Pin,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("pin_cursor", 4, 27, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// InjectPressure
////////////////////////////////////////////////////////////////////////

InjectPressureTool::InjectPressureTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::InjectPressure,
        toolCursorManager,
        gameController,
        soundController)
    , mCurrentAction(std::nullopt)
    , mUpCursorImage(WxHelpers::LoadCursorImage("air_tank_cursor_up", 15, 1, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("air_tank_cursor_down", 15, 1, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// FloodHose
////////////////////////////////////////////////////////////////////////

FloodHoseTool::FloodHoseTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::FloodHose,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mUpCursorImage(WxHelpers::LoadCursorImage("flood_cursor_up", 20, 0, gameAssetManager))
    , mDownCursorImage1(WxHelpers::LoadCursorImage("flood_cursor_down_1", 20, 0, gameAssetManager))
    , mDownCursorImage2(WxHelpers::LoadCursorImage("flood_cursor_down_2", 20, 0, gameAssetManager))
    , mDownCursorCounter(0)
{
}

////////////////////////////////////////////////////////////////////////
// AntiMatterBomb
////////////////////////////////////////////////////////////////////////

AntiMatterBombTool::AntiMatterBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::AntiMatterBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("am_bomb_cursor", 16, 16, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// FireExtinguishingBomb
////////////////////////////////////////////////////////////////////////

FireExtinguishingBombTool::FireExtinguishingBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::FireExtinguishingBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("fire_extinguishing_bomb_cursor", 18, 17, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// ImpactBomb
////////////////////////////////////////////////////////////////////////

ImpactBombTool::ImpactBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::ImpactBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("impact_bomb_cursor", 18, 10, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// RCBomb
////////////////////////////////////////////////////////////////////////

RCBombTool::RCBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::RCBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("rc_bomb_cursor", 16, 21, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// TimerBomb
////////////////////////////////////////////////////////////////////////

TimerBombTool::TimerBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::TimerBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("timer_bomb_cursor", 16, 19, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// WaveMaker
////////////////////////////////////////////////////////////////////////

WaveMakerTool::WaveMakerTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::WaveMaker,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mUpCursorImage(WxHelpers::LoadCursorImage("wave_maker_cursor_up", 15, 15, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("wave_maker_cursor_down", 15, 15, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// TerrainAdjust
////////////////////////////////////////////////////////////////////////

TerrainAdjustTool::TerrainAdjustTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::TerrainAdjust,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mCurrentCursor(nullptr)
    , mUpCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_up", 15, 15, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_down", 15, 15, gameAssetManager))
    , mErrorCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_error", 15, 15, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// Scrub
////////////////////////////////////////////////////////////////////////

ScrubTool::ScrubTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::Scrub,
        toolCursorManager,
        gameController,
        soundController)
    , mScrubUpCursorImage(WxHelpers::LoadCursorImage("scrub_cursor_up", 15, 15, gameAssetManager))
    , mScrubDownCursorImage(WxHelpers::LoadCursorImage("scrub_cursor_down", 15, 15, gameAssetManager))
    , mRotUpCursorImage(WxHelpers::LoadCursorImage("rot_cursor_up", 8, 24, gameAssetManager))
    , mRotDownCursorImage(WxHelpers::LoadCursorImage("rot_cursor_down", 8, 24, gameAssetManager))
    , mPreviousMousePos()
    , mPreviousStrikeVector()
    , mPreviousSoundTimestamp(std::chrono::steady_clock::time_point::min())
{
}

////////////////////////////////////////////////////////////////////////
// Repair Structure
////////////////////////////////////////////////////////////////////////

RepairStructureTool::RepairStructureTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::RepairStructure,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementStartTimestamp()
    , mCurrentStepId()
    , mUpCursorImage(WxHelpers::LoadCursorImage("repair_structure_cursor_up", 8, 8, gameAssetManager))
    , mDownCursorImages{
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_0", 8, 8, gameAssetManager),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_1", 8, 8, gameAssetManager),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_2", 8, 8, gameAssetManager),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_3", 8, 8, gameAssetManager),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_4", 8, 8, gameAssetManager) }
{
}

////////////////////////////////////////////////////////////////////////
// ThanosSnap
////////////////////////////////////////////////////////////////////////

ThanosSnapTool::ThanosSnapTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::ThanosSnap,
        toolCursorManager,
        gameController,
        soundController)
    , mUpNormalCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_up", 15, 15, gameAssetManager))
    , mUpStructuralCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_up_structural", 15, 15, gameAssetManager))
    , mDownNormalCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_down", 15, 15, gameAssetManager))
    , mDownStructuralCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_down_structural", 15, 15, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// ScareFish
////////////////////////////////////////////////////////////////////////

ScareFishTool::ScareFishTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::ScareFish,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mCurrentAction(ActionType::Scare)
    , mScareUpCursorImage(WxHelpers::LoadCursorImage("megaphone_cursor_up", 8, 10, gameAssetManager))
    , mScareDownCursorImage1(WxHelpers::LoadCursorImage("megaphone_cursor_down_1", 8, 21, gameAssetManager))
    , mScareDownCursorImage2(WxHelpers::LoadCursorImage("megaphone_cursor_down_2", 8, 21, gameAssetManager))
    , mAttractUpCursorImage(WxHelpers::LoadCursorImage("food_can_cursor_up", 9, 6, gameAssetManager))
    , mAttractDownCursorImage1(WxHelpers::LoadCursorImage("food_can_cursor_down_1", 8, 21, gameAssetManager))
    , mAttractDownCursorImage2(WxHelpers::LoadCursorImage("food_can_cursor_down_2", 8, 21, gameAssetManager))
    , mDownCursorCounter(0)
{
}

////////////////////////////////////////////////////////////////////////
// PhysicsProbe
////////////////////////////////////////////////////////////////////////

PhysicsProbeTool::PhysicsProbeTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::PhysicsProbe,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("physics_probe_cursor", 0, 19, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// BlastTool
////////////////////////////////////////////////////////////////////////

BlastTool::BlastTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::Blast,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage1(WxHelpers::LoadCursorImage("blast_cursor_up_1", 15, 15, gameAssetManager))
    , mUpCursorImage2(WxHelpers::LoadCursorImage("blast_cursor_up_2", 15, 15, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("empty_cursor", 15, 15, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// ElectricSparkTool
////////////////////////////////////////////////////////////////////////

ElectricSparkTool::ElectricSparkTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::ElectricSpark,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage(WxHelpers::LoadCursorImage("electric_spark_cursor_up", 10, 30, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("electric_spark_cursor_down", 13, 31, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// WindTool
////////////////////////////////////////////////////////////////////////

WindMakerTool::WindMakerTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::WindMaker,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage(WxHelpers::LoadCursorImage("wind_cursor_up", 15, 15, gameAssetManager))
    , mDownCursorImages(
        {
            WxHelpers::LoadCursorImage("wind_cursor_down_1", 15, 15, gameAssetManager),
            WxHelpers::LoadCursorImage("wind_cursor_down_2", 15, 15, gameAssetManager),
            WxHelpers::LoadCursorImage("wind_cursor_down_3", 15, 15, gameAssetManager)
        })
{
}

////////////////////////////////////////////////////////////////////////
// LaserCannonTool
////////////////////////////////////////////////////////////////////////

LaserCannonTool::LaserCannonTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::LaserCannon,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage(WxHelpers::LoadCursorImage("crosshair_cursor_up", 15, 15, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("crosshair_cursor_down", 15, 15, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// LampTool
////////////////////////////////////////////////////////////////////////

LampTool::LampTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::Lamp,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("lamp_cursor_up", 4, 27, gameAssetManager))
    , mDownCursorImage(WxHelpers::LoadCursorImage("lamp_cursor_down", 4, 27, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// LightningStrikeTool
////////////////////////////////////////////////////////////////////////

LightningStrikeTool::LightningStrikeTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : OneShotTool(
        ToolType::LightningStrike,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("lightning_cursor_and_icon", 11, 29, gameAssetManager))
{
}

////////////////////////////////////////////////////////////////////////
// NPCs
////////////////////////////////////////////////////////////////////////

PlaceNpcToolBase::PlaceNpcToolBase(
    ToolType toolType,
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        toolCursorManager,
        gameController,
        soundController)
    , mCurrentEngagementState(std::nullopt)
    , mClosedCursorImage(WxHelpers::LoadCursorImage("place_npc_cursor_down", 11, 29, gameAssetManager))
    , mOpenCursorImage(WxHelpers::LoadCursorImage("place_npc_cursor_up", 11, 29, gameAssetManager))
    , mErrorCursorImage(WxHelpers::LoadCursorImage("place_npc_cursor_error", 11, 29, gameAssetManager))
{}

PlaceFurnitureNpcTool::PlaceFurnitureNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : PlaceNpcToolBase(
        ToolType::PlaceFurnitureNpc,
        toolCursorManager,
        gameController,
        soundController,
        gameAssetManager)
    , mKind() // Will be set later
{
}

PlaceHumanNpcTool::PlaceHumanNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : PlaceNpcToolBase(
        ToolType::PlaceHumanNpc,
        toolCursorManager,
        gameController,
        soundController,
        gameAssetManager)
    , mKind() // Will be set later
{
}

BaseSingleSelectNpcTool::BaseSingleSelectNpcTool(
    ToolType toolType,
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    std::optional<NpcKindType> applicableKind,
    wxImage && downCursorImage,
    wxImage && upCursorImage)
    : Tool(
        toolType,
        toolCursorManager,
        gameController,
        soundController)
    , mApplicableKind(applicableKind)
    , mDownCursorImage(std::move(downCursorImage))
    , mUpCursorImage(std::move(upCursorImage))
{
}

FollowNpcTool::FollowNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : BaseSingleSelectNpcTool(
        ToolType::FollowNpc,
        toolCursorManager,
        gameController,
        soundController,
        std::nullopt, // All kinds
        WxHelpers::LoadCursorImage("autofocus_on_npc_cursor", 16, 16, gameAssetManager),
        WxHelpers::LoadCursorImage("autofocus_on_npc_cursor", 16, 16, gameAssetManager))
{
}

BaseMultiSelectNpcTool::BaseMultiSelectNpcTool(
    ToolType toolType,
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    std::optional<NpcKindType> applicableKind,
    wxImage && downCursorImage,
    wxImage && upCursorImage)
    : Tool(
        toolType,
        toolCursorManager,
        gameController,
        soundController)
    , mApplicableKind(applicableKind)
    , mDownCursorImage(std::move(downCursorImage))
    , mUpCursorImage(std::move(upCursorImage))
{
}

RemoveNpcTool::RemoveNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : BaseMultiSelectNpcTool(
        ToolType::RemoveNpc,
        toolCursorManager,
        gameController,
        soundController,
        std::nullopt, // All kinds
        WxHelpers::LoadCursorImage("remove_npc_cursor_down", 20, 29, gameAssetManager),
        WxHelpers::LoadCursorImage("remove_npc_cursor_up", 20, 29, gameAssetManager))
{
}

TurnaroundNpcTool::TurnaroundNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : BaseMultiSelectNpcTool(
        ToolType::TurnaroundNpc,
        toolCursorManager,
        gameController,
        soundController,
        std::nullopt, // All kinds
        WxHelpers::LoadCursorImage("turnaround_cursor_down", 16, 16, gameAssetManager),
        WxHelpers::LoadCursorImage("turnaround_cursor_up", 16, 16, gameAssetManager))
{
}

MoveNpcTool::MoveNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::MoveNpc,
        toolCursorManager,
        gameController,
        soundController)
    , mCurrentState()
    , mDownCursorImage(WxHelpers::LoadCursorImage("move_npc_cursor_down", 11, 29, gameAssetManager))
    , mUpCursorImage(WxHelpers::LoadCursorImage("move_npc_cursor_up", 11, 29, gameAssetManager))
{
}
