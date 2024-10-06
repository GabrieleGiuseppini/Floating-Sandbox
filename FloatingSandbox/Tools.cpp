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
    ResourceLocator const & resourceLocator)
    : BaseMoveTool(
        ToolType::Move,
        toolCursorManager,
        gameController,
        soundController,
        WxHelpers::LoadCursorImage("move_cursor_up", 13, 5, resourceLocator),
        WxHelpers::LoadCursorImage("move_cursor_down", 13, 5, resourceLocator),
        WxHelpers::LoadCursorImage("move_cursor_rotate_up", 13, 5, resourceLocator),
        WxHelpers::LoadCursorImage("move_cursor_rotate_down", 13, 5, resourceLocator))
{
}

MoveAllTool::MoveAllTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : BaseMoveTool(
        ToolType::MoveAll,
        toolCursorManager,
        gameController,
        soundController,
        WxHelpers::LoadCursorImage("move_all_cursor_up", 13, 5, resourceLocator),
        WxHelpers::LoadCursorImage("move_all_cursor_down", 13, 5, resourceLocator),
        WxHelpers::LoadCursorImage("move_all_cursor_rotate_up", 13, 5, resourceLocator),
        WxHelpers::LoadCursorImage("move_all_cursor_rotate_down", 13, 5, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// Pick and Pull
////////////////////////////////////////////////////////////////////////

PickAndPullTool::PickAndPullTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::PickAndPull,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("pliers_cursor_up", 2, 2, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("pliers_cursor_down", 2, 2, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// Smash
////////////////////////////////////////////////////////////////////////

SmashTool::SmashTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : ContinuousTool(
        ToolType::Smash,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("smash_cursor_up", 6, 9, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("smash_cursor_down", 6, 9, resourceLocator))
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
        radiusFraction * 10.0f);
}

////////////////////////////////////////////////////////////////////////
// Saw
////////////////////////////////////////////////////////////////////////

SawTool::SawTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::Saw,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("chainsaw_cursor_up", 8, 20, resourceLocator))
    , mDownCursorImage1(WxHelpers::LoadCursorImage("chainsaw_cursor_down_1", 8, 20, resourceLocator))
    , mDownCursorImage2(WxHelpers::LoadCursorImage("chainsaw_cursor_down_2", 8, 20, resourceLocator))
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
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::HeatBlaster,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mCurrentAction(HeatBlasterActionType::Heat)
    , mHeatUpCursorImage(WxHelpers::LoadCursorImage("heat_blaster_heat_cursor_up", 5, 1, resourceLocator))
    , mCoolUpCursorImage(WxHelpers::LoadCursorImage("heat_blaster_cool_cursor_up", 5, 30, resourceLocator))
    , mHeatDownCursorImage(WxHelpers::LoadCursorImage("heat_blaster_heat_cursor_down", 5, 1, resourceLocator))
    , mCoolDownCursorImage(WxHelpers::LoadCursorImage("heat_blaster_cool_cursor_down", 5, 30, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// FireExtinguisher
////////////////////////////////////////////////////////////////////////

FireExtinguisherTool::FireExtinguisherTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::FireExtinguisher,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mUpCursorImage(WxHelpers::LoadCursorImage("fire_extinguisher_cursor_up", 6, 3, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("fire_extinguisher_cursor_down", 6, 3, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// Grab
////////////////////////////////////////////////////////////////////////

GrabTool::GrabTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : ContinuousTool(
        ToolType::Grab,
        toolCursorManager,
        gameController,
        soundController)
    , mUpPlusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_up_plus", 15, 15, resourceLocator))
    , mUpMinusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_up_minus", 15, 15, resourceLocator))
    , mDownPlusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_down_plus", 15, 15, resourceLocator))
    , mDownMinusCursorImage(WxHelpers::LoadCursorImage("drag_cursor_down_minus", 15, 15, resourceLocator))
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
    ResourceLocator const & resourceLocator)
    : ContinuousTool(
        ToolType::Grab,
        toolCursorManager,
        gameController,
        soundController)
    , mUpPlusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_up_cw", 15, 15, resourceLocator))
    , mUpMinusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_up_ccw", 15, 15, resourceLocator))
    , mDownPlusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_down_cw", 15, 15, resourceLocator))
    , mDownMinusCursorImage(WxHelpers::LoadCursorImage("swirl_cursor_down_ccw", 15, 15, resourceLocator))
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

    // Draw
    mGameController.SwirlAt(
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
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::Pin,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("pin_cursor", 4, 27, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// InjectPressure
////////////////////////////////////////////////////////////////////////

InjectPressureTool::InjectPressureTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::InjectPressure,
        toolCursorManager,
        gameController,
        soundController)
    , mCurrentAction(std::nullopt)
    , mUpCursorImage(WxHelpers::LoadCursorImage("air_tank_cursor_up", 15, 1, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("air_tank_cursor_down", 15, 1, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// FloodHose
////////////////////////////////////////////////////////////////////////

FloodHoseTool::FloodHoseTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::FloodHose,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mUpCursorImage(WxHelpers::LoadCursorImage("flood_cursor_up", 20, 0, resourceLocator))
    , mDownCursorImage1(WxHelpers::LoadCursorImage("flood_cursor_down_1", 20, 0, resourceLocator))
    , mDownCursorImage2(WxHelpers::LoadCursorImage("flood_cursor_down_2", 20, 0, resourceLocator))
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
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::AntiMatterBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("am_bomb_cursor", 16, 16, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// ImpactBomb
////////////////////////////////////////////////////////////////////////

ImpactBombTool::ImpactBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::ImpactBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("impact_bomb_cursor", 18, 10, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// RCBomb
////////////////////////////////////////////////////////////////////////

RCBombTool::RCBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::RCBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("rc_bomb_cursor", 16, 21, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// TimerBomb
////////////////////////////////////////////////////////////////////////

TimerBombTool::TimerBombTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::TimerBomb,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("timer_bomb_cursor", 16, 19, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// WaveMaker
////////////////////////////////////////////////////////////////////////

WaveMakerTool::WaveMakerTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::WaveMaker,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mUpCursorImage(WxHelpers::LoadCursorImage("wave_maker_cursor_up", 15, 15, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("wave_maker_cursor_down", 15, 15, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// TerrainAdjust
////////////////////////////////////////////////////////////////////////

TerrainAdjustTool::TerrainAdjustTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::TerrainAdjust,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mCurrentCursor(nullptr)
    , mUpCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_up", 15, 15, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_down", 15, 15, resourceLocator))
    , mErrorCursorImage(WxHelpers::LoadCursorImage("terrain_adjust_cursor_error", 15, 15, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// Scrub
////////////////////////////////////////////////////////////////////////

ScrubTool::ScrubTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::Scrub,
        toolCursorManager,
        gameController,
        soundController)
    , mScrubUpCursorImage(WxHelpers::LoadCursorImage("scrub_cursor_up", 15, 15, resourceLocator))
    , mScrubDownCursorImage(WxHelpers::LoadCursorImage("scrub_cursor_down", 15, 15, resourceLocator))
    , mRotUpCursorImage(WxHelpers::LoadCursorImage("rot_cursor_up", 8, 24, resourceLocator))
    , mRotDownCursorImage(WxHelpers::LoadCursorImage("rot_cursor_down", 8, 24, resourceLocator))
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
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::RepairStructure,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementStartTimestamp()
    , mCurrentStepId()
    , mUpCursorImage(WxHelpers::LoadCursorImage("repair_structure_cursor_up", 8, 8, resourceLocator))
    , mDownCursorImages{
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_0", 8, 8, resourceLocator),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_1", 8, 8, resourceLocator),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_2", 8, 8, resourceLocator),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_3", 8, 8, resourceLocator),
        WxHelpers::LoadCursorImage("repair_structure_cursor_down_4", 8, 8, resourceLocator) }
{
}

////////////////////////////////////////////////////////////////////////
// ThanosSnap
////////////////////////////////////////////////////////////////////////

ThanosSnapTool::ThanosSnapTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::ThanosSnap,
        toolCursorManager,
        gameController,
        soundController)
    , mUpNormalCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_up", 15, 15, resourceLocator))
    , mUpStructuralCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_up_structural", 15, 15, resourceLocator))
    , mDownNormalCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_down", 15, 15, resourceLocator))
    , mDownStructuralCursorImage(WxHelpers::LoadCursorImage("thanos_snap_cursor_down_structural", 15, 15, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// ScareFish
////////////////////////////////////////////////////////////////////////

ScareFishTool::ScareFishTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::ScareFish,
        toolCursorManager,
        gameController,
        soundController)
    , mIsEngaged(false)
    , mCurrentAction(ActionType::Scare)
    , mScareUpCursorImage(WxHelpers::LoadCursorImage("megaphone_cursor_up", 8, 10, resourceLocator))
    , mScareDownCursorImage1(WxHelpers::LoadCursorImage("megaphone_cursor_down_1", 8, 21, resourceLocator))
    , mScareDownCursorImage2(WxHelpers::LoadCursorImage("megaphone_cursor_down_2", 8, 21, resourceLocator))
    , mAttractUpCursorImage(WxHelpers::LoadCursorImage("food_can_cursor_up", 9, 6, resourceLocator))
    , mAttractDownCursorImage1(WxHelpers::LoadCursorImage("food_can_cursor_down_1", 8, 21, resourceLocator))
    , mAttractDownCursorImage2(WxHelpers::LoadCursorImage("food_can_cursor_down_2", 8, 21, resourceLocator))
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
    ResourceLocator const & resourceLocator)
    : OneShotTool(
        ToolType::PhysicsProbe,
        toolCursorManager,
        gameController,
        soundController)
    , mCursorImage(WxHelpers::LoadCursorImage("physics_probe_cursor", 0, 19, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// BlastTool
////////////////////////////////////////////////////////////////////////

BlastTool::BlastTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::Blast,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage1(WxHelpers::LoadCursorImage("blast_cursor_up_1", 15, 15, resourceLocator))
    , mUpCursorImage2(WxHelpers::LoadCursorImage("blast_cursor_up_2", 15, 15, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("empty_cursor", 15, 15, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// ElectricSparkTool
////////////////////////////////////////////////////////////////////////

ElectricSparkTool::ElectricSparkTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::ElectricSpark,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage(WxHelpers::LoadCursorImage("electric_spark_cursor_up", 10, 30, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("electric_spark_cursor_down", 13, 31, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// WindTool
////////////////////////////////////////////////////////////////////////

WindMakerTool::WindMakerTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::WindMaker,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage(WxHelpers::LoadCursorImage("wind_cursor_up", 15, 15, resourceLocator))
    , mDownCursorImages(
        {
            WxHelpers::LoadCursorImage("wind_cursor_down_1", 15, 15, resourceLocator),
            WxHelpers::LoadCursorImage("wind_cursor_down_2", 15, 15, resourceLocator),
            WxHelpers::LoadCursorImage("wind_cursor_down_3", 15, 15, resourceLocator)
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
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::LaserCannon,
        toolCursorManager,
        gameController,
        soundController)
    , mEngagementData()
    , mUpCursorImage(WxHelpers::LoadCursorImage("crosshair_cursor_up", 15, 15, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("crosshair_cursor_down", 15, 15, resourceLocator))
{
}

////////////////////////////////////////////////////////////////////////
// LampTool
////////////////////////////////////////////////////////////////////////

LampTool::LampTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::Lamp,
        toolCursorManager,
        gameController,
        soundController)
    , mUpCursorImage(WxHelpers::LoadCursorImage("lamp_cursor_up", 4, 27, resourceLocator))
    , mDownCursorImage(WxHelpers::LoadCursorImage("lamp_cursor_down", 4, 27, resourceLocator))
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
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        toolCursorManager,
        gameController,
        soundController)
    , mCurrentEngagementState(std::nullopt)
    , mClosedCursorImage(WxHelpers::LoadCursorImage("place_npc_cursor_down", 11, 29, resourceLocator))
    , mOpenCursorImage(WxHelpers::LoadCursorImage("place_npc_cursor_up", 11, 29, resourceLocator))
    , mErrorCursorImage(WxHelpers::LoadCursorImage("place_npc_cursor_error", 11, 29, resourceLocator))
{}

PlaceFurnitureNpcTool::PlaceFurnitureNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : PlaceNpcToolBase(
        ToolType::PlaceFurnitureNpc,
        toolCursorManager,
        gameController,
        soundController,
        resourceLocator)
    , mKind(0) // Will be set later
{
}

PlaceHumanNpcTool::PlaceHumanNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : PlaceNpcToolBase(
        ToolType::PlaceHumanNpc,
        toolCursorManager,
        gameController,
        soundController,
        resourceLocator)
    , mKind(0) // Will be set later
{
}

BaseSelectNpcTool::BaseSelectNpcTool(
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

MoveNpcTool::MoveNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : BaseSelectNpcTool(
        ToolType::MoveNpc,
        toolCursorManager,
        gameController,
        soundController,
        std::nullopt, // All kinds
        WxHelpers::LoadCursorImage("move_npc_cursor_down", 11, 29, resourceLocator),
        WxHelpers::LoadCursorImage("move_npc_cursor_up", 11, 29, resourceLocator))
    , mBeingMovedNpc()
{
}

RemoveNpcTool::RemoveNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : BaseSelectNpcTool(
        ToolType::RemoveNpc,
        toolCursorManager,
        gameController,
        soundController,
        std::nullopt, // All kinds
        WxHelpers::LoadCursorImage("remove_npc_cursor_down", 20, 29, resourceLocator),
        WxHelpers::LoadCursorImage("remove_npc_cursor_up", 20, 29, resourceLocator))
{
}

TurnaroundNpcTool::TurnaroundNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : BaseSelectNpcTool(
        ToolType::TurnaroundNpc,
        toolCursorManager,
        gameController,
        soundController,
        std::nullopt, // All kinds
        WxHelpers::LoadCursorImage("turnaround_cursor_down", 16, 16, resourceLocator),
        WxHelpers::LoadCursorImage("turnaround_cursor_up", 16, 16, resourceLocator))
{
}

FollowNpcTool::FollowNpcTool(
    IToolCursorManager & toolCursorManager,
    IGameController & gameController,
    SoundController & soundController,
    ResourceLocator const & resourceLocator)
    : BaseSelectNpcTool(
        ToolType::FollowNpc,
        toolCursorManager,
        gameController,
        soundController,
        std::nullopt, // All kinds
        WxHelpers::LoadCursorImage("autofocus_on_npc_cursor", 16, 16, resourceLocator),
        WxHelpers::LoadCursorImage("autofocus_on_npc_cursor", 16, 16, resourceLocator))
{
}
