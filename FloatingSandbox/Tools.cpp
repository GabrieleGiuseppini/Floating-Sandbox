/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tools.h"

#include <GameLib/GameException.h>

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
        auto now = std::chrono::steady_clock::now();

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
    // Calculate radius multiplier
    // 0-500ms      = 1.0
    // 5000ms-+INF = 10.0

    static constexpr float MaxMultiplier = 10.0f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float radiusMultiplier = 1.0f + (MaxMultiplier - 1.0f) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    ModulateCursor(mDownCursors, radiusMultiplier, 1.0f, MaxMultiplier);

    // Destroy
    mGameController->DestroyAt(
        inputState.MousePosition, 
        radiusMultiplier);
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
    // 0-500ms      = 1.0
    // 5000ms-+INF = 20.0

    static constexpr float MaxMultiplier = 20.0f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float strengthMultiplier = 1.0f + (MaxMultiplier - 1.0f) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    ModulateCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursors : mDownPlusCursors, 
        strengthMultiplier, 
        1.0f, 
        MaxMultiplier);

    // Draw
    mGameController->DrawTo(
        inputState.MousePosition,
        inputState.IsShiftKeyDown
            ? -strengthMultiplier
            : strengthMultiplier);
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
    // 0-500ms      = 1.0
    // 5000ms-+INF = 20.0

    static constexpr float MaxMultiplier = 20.0f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float strengthMultiplier = 1.0f + (MaxMultiplier - 1.0f) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    ModulateCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursors : mDownPlusCursors,
        strengthMultiplier,
        1.0f,
        MaxMultiplier);

    // Draw
    mGameController->SwirlAt(
        inputState.MousePosition,
        inputState.IsShiftKeyDown
            ? -strengthMultiplier
            : strengthMultiplier);
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
