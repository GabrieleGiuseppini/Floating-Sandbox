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

static constexpr int CursorStep = 30;

std::vector<std::unique_ptr<wxCursor>> MakeCursors(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader)
{
    auto filepath = resourceLoader.GetCursorFilepath(cursorName);
    wxBitmap* bmp = new wxBitmap(filepath.string(), wxBITMAP_TYPE_PNG);
    if (nullptr == bmp)
    {
        throw GameException("Cannot load resource '" + filepath.string() + "'");
    }

    wxImage img = bmp->ConvertToImage();
    int const imageWidth = img.GetWidth();
    int const imageHeight = img.GetHeight();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    //
    // Build series of cursors for each power step: 0 (base), 1-CursorStep
    //

    std::vector<std::unique_ptr<wxCursor>> cursors;

    // Create base
    cursors.emplace_back(std::make_unique<wxCursor>(img));

    // Create power steps

    static_assert(CursorStep > 0, "CursorStep is at least 1");

    unsigned char * data = img.GetData();

    for (int c = 1; c <= CursorStep; ++c)
    {
        int powerHeight = static_cast<int>(floorf(
            static_cast<float>(imageHeight) * static_cast<float>(c) / static_cast<float>(CursorStep)
        ));

        // Start from top
        for (int y = imageHeight - powerHeight; y < imageHeight; ++y)
        {
            int rowStartIndex = (imageWidth * y);

            // Red   = 0xDB0F0F
            // Green = 0x039B0A (final)

            float const targetR = ((c == CursorStep) ? 0x03 : 0xDB) / 255.0f;
            float const targetG = ((c == CursorStep) ? 0x9B : 0x0F) / 255.0f;
            float const targetB = ((c == CursorStep) ? 0x0A : 0x0F) / 255.0f;

            for (int x = 0; x < imageWidth; ++x)
            {
                data[(rowStartIndex + x) * 3] = static_cast<unsigned char>(targetR * 255.0f);
                data[(rowStartIndex + x) * 3 + 1] = static_cast<unsigned char>(targetG * 255.0f);
                data[(rowStartIndex + x) * 3 + 2] = static_cast<unsigned char>(targetB * 255.0f);
            }
        }

        cursors.emplace_back(std::make_unique<wxCursor>(img));
    }

    delete (bmp);

    return cursors;
}

std::unique_ptr<wxCursor> MakeCursor(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader)
{
    auto filepath = resourceLoader.GetCursorFilepath(cursorName);
    wxBitmap* bmp = new wxBitmap(filepath.string(), wxBITMAP_TYPE_PNG);
    if (nullptr == bmp)
    {
        throw GameException("Cannot load resource '" + filepath.string() + "'");
    }

    wxImage img = bmp->ConvertToImage();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    return std::make_unique<wxCursor>(img);
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
        auto now = GameWallClock::GetInstance().Now();

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
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::Move,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mEngagedElementId(std::nullopt)
    , mCurrentTrajectory(std::nullopt)
    , mRotationCenter(std::nullopt)
    , mUpCursor(MakeCursor("move_cursor_up", 13, 5, resourceLoader))
    , mDownCursor(MakeCursor("move_cursor_down", 13, 5, resourceLoader))
    , mRotateUpCursor(MakeCursor("move_cursor_rotate_up", 13, 5, resourceLoader))
    , mRotateDownCursor(MakeCursor("move_cursor_rotate_down", 13, 5, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// MoveAll
////////////////////////////////////////////////////////////////////////

MoveAllTool::MoveAllTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::MoveAll,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mEngagedShipId(std::nullopt)
    , mCurrentTrajectory(std::nullopt)
    , mRotationCenter(std::nullopt)
    , mUpCursor(MakeCursor("move_all_cursor_up", 13, 5, resourceLoader))
    , mDownCursor(MakeCursor("move_all_cursor_down", 13, 5, resourceLoader))
    , mRotateUpCursor(MakeCursor("move_all_cursor_rotate_up", 13, 5, resourceLoader))
    , mRotateDownCursor(MakeCursor("move_all_cursor_rotate_down", 13, 5, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// Smash
////////////////////////////////////////////////////////////////////////

SmashTool::SmashTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Smash,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mUpCursor(MakeCursor("smash_cursor_up", 6, 9, resourceLoader))
    , mDownCursors(MakeCursors("smash_cursor_down", 6, 9, resourceLoader))
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
    ModulateCursor(mDownCursors, radiusFraction);

    // Destroy
    mGameController->DestroyAt(
        inputState.MousePosition,
        radiusFraction);
}

////////////////////////////////////////////////////////////////////////
// Saw
////////////////////////////////////////////////////////////////////////

SawTool::SawTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::Saw,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mUpCursor(MakeCursor("chainsaw_cursor_up", 8, 20, resourceLoader))
    , mDownCursor1(MakeCursor("chainsaw_cursor_down_1", 8, 20, resourceLoader))
    , mDownCursor2(MakeCursor("chainsaw_cursor_down_2", 8, 20, resourceLoader))
    , mCurrentCursor(nullptr)
    , mPreviousMousePos()
    , mDownCursorCounter(0)
    , mIsUnderwater(false)
{
}

////////////////////////////////////////////////////////////////////////
// Grab
////////////////////////////////////////////////////////////////////////

GrabTool::GrabTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Grab,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mUpPlusCursor(MakeCursor("drag_cursor_up_plus", 15, 15, resourceLoader))
    , mUpMinusCursor(MakeCursor("drag_cursor_up_minus", 15, 15, resourceLoader))
    , mDownPlusCursors(MakeCursors("drag_cursor_down_plus", 15, 15, resourceLoader))
    , mDownMinusCursors(MakeCursors("drag_cursor_down_minus", 15, 15, resourceLoader))
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
    ModulateCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursors : mDownPlusCursors,
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
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Grab,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mUpPlusCursor(MakeCursor("swirl_cursor_up_cw", 15, 15, resourceLoader))
    , mUpMinusCursor(MakeCursor("swirl_cursor_up_ccw", 15, 15, resourceLoader))
    , mDownPlusCursors(MakeCursors("swirl_cursor_down_cw", 15, 15, resourceLoader))
    , mDownMinusCursors(MakeCursors("swirl_cursor_down_ccw", 15, 15, resourceLoader))
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
    ModulateCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursors : mDownPlusCursors,
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
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::Pin,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mCursor(MakeCursor("pin_cursor", 4, 27, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// InjectAirBubbles
////////////////////////////////////////////////////////////////////////

InjectAirBubblesTool::InjectAirBubblesTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::InjectAirBubbles,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mIsEngaged(false)
    , mUpCursor(MakeCursor("air_tank_cursor_up", 12, 1, resourceLoader))
    , mDownCursor(MakeCursor("air_tank_cursor_down", 12, 1, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// FloodHose
////////////////////////////////////////////////////////////////////////

FloodHoseTool::FloodHoseTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::FloodHose,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mIsEngaged(false)
    , mUpCursor(MakeCursor("flood_cursor_up", 20, 0, resourceLoader))
    , mDownCursor1(MakeCursor("flood_cursor_down_1", 20, 0, resourceLoader))
    , mDownCursor2(MakeCursor("flood_cursor_down_2", 20, 0, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// AntiMatterBomb
////////////////////////////////////////////////////////////////////////

AntiMatterBombTool::AntiMatterBombTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::AntiMatterBomb,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mCursor(MakeCursor("am_bomb_cursor", 16, 16, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// ImpactBomb
////////////////////////////////////////////////////////////////////////

ImpactBombTool::ImpactBombTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::ImpactBomb,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mCursor(MakeCursor("impact_bomb_cursor", 18, 10, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// RCBomb
////////////////////////////////////////////////////////////////////////

RCBombTool::RCBombTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::RCBomb,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mCursor(MakeCursor("rc_bomb_cursor", 16, 21, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// TimerBomb
////////////////////////////////////////////////////////////////////////

TimerBombTool::TimerBombTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::TimerBomb,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mCursor(MakeCursor("timer_bomb_cursor", 16, 19, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// GenerateWave
////////////////////////////////////////////////////////////////////////

GenerateWaveTool::GenerateWaveTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::GenerateWave,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mCurrentTrajectory()
    , mCurrentCursor(nullptr)
    , mUpCursor(MakeCursor("generate_wave_cursor_up", 15, 15, resourceLoader))
    , mDownCursor(MakeCursor("generate_wave_cursor_down", 15, 15, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// TerrainAdjust
////////////////////////////////////////////////////////////////////////

TerrainAdjustTool::TerrainAdjustTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::TerrainAdjust,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mCurrentTrajectoryPreviousPosition()
    , mUpCursor(MakeCursor("terrain_adjust_cursor_up", 15, 15, resourceLoader))
    , mDownCursor(MakeCursor("terrain_adjust_cursor_down", 15, 15, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// Scrub
////////////////////////////////////////////////////////////////////////

ScrubTool::ScrubTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::Scrub,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mUpCursor(MakeCursor("scrub_cursor_up", 15, 15, resourceLoader))
    , mDownCursor(MakeCursor("scrub_cursor_down", 15, 15, resourceLoader))
    , mPreviousMousePos()
    , mPreviousScrub()
    , mPreviousScrubTimestamp(GameWallClock::time_point::min())
{
}

////////////////////////////////////////////////////////////////////////
// Scrub
////////////////////////////////////////////////////////////////////////

RepairStructureTool::RepairStructureTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader & resourceLoader)
    : Tool(
        ToolType::RepairStructure,
        parentFrame,
        std::move(gameController),
        std::move(soundController))
    , mEngagementStartTimestamp()
    , mCurrentCursor(nullptr)
    , mUpCursor(MakeCursor("repair_structure_cursor_up", 8, 8, resourceLoader))
    , mDownCursors{
        MakeCursor("repair_structure_cursor_down_0", 8, 8, resourceLoader),
        MakeCursor("repair_structure_cursor_down_1", 8, 8, resourceLoader),
        MakeCursor("repair_structure_cursor_down_2", 8, 8, resourceLoader),
        MakeCursor("repair_structure_cursor_down_3", 8, 8, resourceLoader),
        MakeCursor("repair_structure_cursor_down_4", 8, 8, resourceLoader) }
{
}