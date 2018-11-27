/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"

#include <GameLib/GameController.h>
#include <GameLib/ResourceLoader.h>

#include <wx/cursor.h>
#include <wx/frame.h>

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

std::vector<std::unique_ptr<wxCursor>> MakeCursors(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader);

std::unique_ptr<wxCursor> MakeCursor(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader);

enum class ToolType
{
    Move = 0,
    Smash = 1,
    Saw = 2,
    Grab = 3,
    Swirl = 4,
    Pin = 5,
    TimerBomb = 6,
    RCBomb = 7,
    AntiMatterBomb = 8
};

struct InputState
{
    bool IsLeftMouseDown;
    bool IsRightMouseDown;
    bool IsShiftKeyDown;
    vec2f MousePosition;
    vec2f PreviousMousePosition;

    InputState()
        : IsLeftMouseDown(false)
        , IsRightMouseDown(false)
        , IsShiftKeyDown(false)
        , MousePosition()
        , PreviousMousePosition()
    {
    }
};

/*
 * Base abstract class of all tools.
 */
class Tool
{
public:

    virtual ~Tool() {}

    ToolType GetToolType() const { return mToolType; }

    virtual void Initialize(InputState const & inputState) = 0;
    virtual void Deinitialize(InputState const & inputState) = 0;

    virtual void Update(InputState const & inputState) = 0;

    virtual void OnMouseMove(InputState const & inputState) = 0;
    virtual void OnLeftMouseDown(InputState const & inputState) = 0;
    virtual void OnLeftMouseUp(InputState const & inputState) = 0;
    virtual void OnShiftKeyDown(InputState const & inputState) = 0;
    virtual void OnShiftKeyUp(InputState const & inputState) = 0;

    virtual void ShowCurrentCursor() = 0;

protected:

    Tool(
        ToolType toolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController)
        : mToolType(toolType)
        , mParentFrame(parentFrame)
        , mGameController(gameController)
        , mSoundController(soundController)
    {}

    wxFrame * const mParentFrame;
    std::shared_ptr<GameController> const mGameController;
    std::shared_ptr<SoundController> const mSoundController;

private:

    ToolType const mToolType;
};

/*
 * Base class of tools that shoot once.
 */
class OneShotTool : public Tool
{
public:

    virtual ~OneShotTool() {}

    virtual void Deinitialize(InputState const & /*inputState*/) override {}

    virtual void Update(InputState const & /*inputState*/) override {}

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);
        assert(nullptr != mCurrentCursor);

        mParentFrame->SetCursor(*mCurrentCursor);
    }

protected:

    OneShotTool(
        ToolType toolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController)
        : Tool(
            toolType,
            parentFrame,
            std::move(gameController),
            std::move(soundController))
        , mCurrentCursor(nullptr)
    {}

protected:

    wxCursor * mCurrentCursor;
};

/*
 * Base class of tools that apply continuously.
 */
class ContinuousTool : public Tool
{
public:

    virtual ~ContinuousTool() {}

    virtual void Initialize(InputState const & inputState) override
    {
        // Initialize the continuous tool state
        mPreviousMousePosition = inputState.MousePosition;
        mPreviousTimestamp = std::chrono::steady_clock::now();
        mCumulatedTime = std::chrono::microseconds(0);
    }

    virtual void Update(InputState const & inputState) override;

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Initialize the continuous tool state
        mPreviousMousePosition = inputState.MousePosition;
        mPreviousTimestamp = std::chrono::steady_clock::now();
        mCumulatedTime = std::chrono::microseconds(0);
    }

    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);
        assert(nullptr != mCurrentCursor);

        mParentFrame->SetCursor(*mCurrentCursor);
    }

protected:

    ContinuousTool(
        ToolType toolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController)
        : Tool(
            toolType,
            parentFrame,
            std::move(gameController),
            std::move(soundController))
        , mCurrentCursor(nullptr)
        , mPreviousMousePosition()
        , mPreviousTimestamp(std::chrono::steady_clock::now())
        , mCumulatedTime(0)
    {}

    void ModulateCursor(
        std::vector<std::unique_ptr<wxCursor>> const & cursors,
        float strength,
        float minStrength,
        float maxStrength)
    {
        // Calculate cursor index (cursor 0 is the base, we don't use it here)
        size_t const numberOfCursors = (cursors.size() - 1);
        size_t cursorIndex = 1u + static_cast<size_t>(
            floorf(
                (strength - minStrength) / (maxStrength - minStrength) * static_cast<float>(numberOfCursors - 1)));

        // Set cursor
        mCurrentCursor = cursors[cursorIndex].get();

        // Display cursor
        ShowCurrentCursor();
    }

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) = 0;

protected:

    // The currently-selected cursor that will be shown
    wxCursor * mCurrentCursor;

private:

    //
    // Tool state
    //

    // Previous mouse position and time when we looked at it
    vec2f mPreviousMousePosition;
    std::chrono::steady_clock::time_point mPreviousTimestamp;

    // The total accumulated press time - the proxy for the strength of the tool
    std::chrono::microseconds mCumulatedTime;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Tools
//////////////////////////////////////////////////////////////////////////////////////////

class MoveTool final : public OneShotTool
{
public:

    MoveTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset state
        mEngagedShipId.reset();

        // Reset cursor
        assert(!!mUpCursor);
        mCurrentCursor = mUpCursor.get();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        if (!!mEngagedShipId)
        {
            // Tell GameController
            mGameController->SetMoveToolEngaged(false);
        }
    }

    virtual void OnMouseMove(InputState const & inputState) override
    {
        if (!!mEngagedShipId)
        {
            // Tell GameController
            vec2f screenOffset = inputState.MousePosition - inputState.PreviousMousePosition;
            mGameController->MoveBy(*mEngagedShipId, screenOffset);
        }
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Check with game controller
        auto pointId = mGameController->GetNearestPointAt(inputState.MousePosition);
        if (!!pointId)
        {
            // Engaged
            mEngagedShipId = pointId->GetShipId();

            // Set cursor
            assert(!!mDownCursor);
            mCurrentCursor = mDownCursor.get();
            ShowCurrentCursor();

            // Tell GameController
            mGameController->SetMoveToolEngaged(true);
        }
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override
    {
        if (!!mEngagedShipId)
        {
            // Disengage
            mEngagedShipId.reset();

            // Set cursor
            assert(!!mUpCursor);
            mCurrentCursor = mUpCursor.get();
            ShowCurrentCursor();

            // Tell GameController
            mGameController->SetMoveToolEngaged(false);
        }
    }

private:

    // When engaged, the ID of the ship we're currently moving
    std::optional<ShipId> mEngagedShipId;

    // The up cursor
    std::unique_ptr<wxCursor> const mUpCursor;

    // The down cursor
    std::unique_ptr<wxCursor> const mDownCursor;
};

class SmashTool final : public ContinuousTool
{
public:

    SmashTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        ContinuousTool::Initialize(inputState);

        // Reset current cursor
        if (inputState.IsLeftMouseDown)
        {
            // Down
            mCurrentCursor = mDownCursors[0].get();
        }
        else
        {
            // Up
            mCurrentCursor = mUpCursor.get();
        }
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        mCurrentCursor = mUpCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        ContinuousTool::OnLeftMouseDown(inputState);

        // Set current cursor to the first down cursor
        mCurrentCursor = mDownCursors[0].get();
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override
    {
        // Set current cursor to the up cursor
        mCurrentCursor = mUpCursor.get();
        ShowCurrentCursor();
    }

protected:

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) override;

private:

    // The up cursor
    std::unique_ptr<wxCursor> const mUpCursor;

    // The force-modulated down cursors;
    // cursor 0 is base; cursor 1 up to size-1 are strength-based
    std::vector<std::unique_ptr<wxCursor>> const mDownCursors;
};

class SawTool final : public Tool
{
public:

    SawTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            // Initialize state
            mPreviousMousePos = inputState.MousePosition;
            mIsUnderwater = mGameController->IsUnderwater(inputState.MousePosition);

            // Start sound
            mSoundController->PlaySawSound(mIsUnderwater);

            // Set current cursor to the current down cursor
            mCurrentCursor = (mDownCursorCounter % 2) ? mDownCursor2.get() : mDownCursor1.get();
        }
        else
        {
            // Reset state
            mPreviousMousePos = std::nullopt;

            // Set current cursor to the up cursor
            mCurrentCursor = mUpCursor.get();
        }
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound
        mSoundController->StopSawSound();
    }

    virtual void Update(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            // Update underwater-ness
            bool isUnderwater = mGameController->IsUnderwater(inputState.MousePosition);
            if (isUnderwater != mIsUnderwater)
            {
                // Update sound
                mSoundController->PlaySawSound(isUnderwater);

                // Update state
                mIsUnderwater = isUnderwater;
            }

            // Update down cursor
            ++mDownCursorCounter;
            mCurrentCursor = (mDownCursorCounter % 2) ? mDownCursor2.get() : mDownCursor1.get();
            ShowCurrentCursor();
        }
    }

    virtual void OnMouseMove(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            if (!!mPreviousMousePos)
            {
                mGameController->SawThrough(
                    *mPreviousMousePos,
                    inputState.MousePosition);
            }

            // Remember the next previous mouse position
            mPreviousMousePos = inputState.MousePosition;
        }
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Initialize state
        mPreviousMousePos = inputState.MousePosition;
        mIsUnderwater = mGameController->IsUnderwater(inputState.MousePosition);

        // Start sound
        mSoundController->PlaySawSound(mGameController->IsUnderwater(inputState.MousePosition));

        // Set current cursor to the current down cursor
        mCurrentCursor = (mDownCursorCounter % 2) ? mDownCursor2.get() : mDownCursor1.get();
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override
    {
        // Reset state
        mPreviousMousePos = std::nullopt;

        // Stop sound
        mSoundController->StopSawSound();

        // Set current cursor to the up cursor
        mCurrentCursor = mUpCursor.get();
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);
        assert(nullptr != mCurrentCursor);

        mParentFrame->SetCursor(*mCurrentCursor);
    }

private:

    // Our cursors
    std::unique_ptr<wxCursor> const mUpCursor;
    std::unique_ptr<wxCursor> const mDownCursor1;
    std::unique_ptr<wxCursor> const mDownCursor2;

    // The currently-selected cursor that will be shown
    wxCursor * mCurrentCursor;

    //
    // State
    //

    // The previous mouse position; when set, we have a segment and can saw
    std::optional<vec2f> mPreviousMousePos;

    // The current counter for the down cursors
    uint8_t mDownCursorCounter;

    // The current above/underwaterness of the tool
    bool mIsUnderwater;
};

class GrabTool final : public ContinuousTool
{
public:

    GrabTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        ContinuousTool::Initialize(inputState);

        if (inputState.IsLeftMouseDown)
        {
            // Start sound
            mSoundController->PlayDrawSound(mGameController->IsUnderwater(inputState.MousePosition));
        }

        SetBasisCursor(inputState);
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound
        mSoundController->StopDrawSound();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        ContinuousTool::OnLeftMouseDown(inputState);

        // Start sound
        mSoundController->PlayDrawSound(mGameController->IsUnderwater(inputState.MousePosition));

        // Change cursor
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Stop sound
        mSoundController->StopDrawSound();

        // Change cursor
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyDown(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyUp(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

protected:

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) override;

private:

    void SetBasisCursor(InputState const & inputState)
    {
        if (inputState.IsLeftMouseDown)
        {
            if (inputState.IsShiftKeyDown)
            {
                // Down minus
                mCurrentCursor = mDownMinusCursors[0].get();
            }
            else
            {
                // Down plus
                mCurrentCursor = mDownPlusCursors[0].get();
            }
        }
        else
        {
            if (inputState.IsShiftKeyDown)
            {
                // Up minus
                mCurrentCursor = mUpMinusCursor.get();
            }
            else
            {
                // Up plus
                mCurrentCursor = mUpPlusCursor.get();
            }
        }
    }

    // The up cursors
    std::unique_ptr<wxCursor> const mUpPlusCursor;
    std::unique_ptr<wxCursor> const mUpMinusCursor;

    // The force-modulated down cursors;
    // cursor 0 is base; cursor 1 up to size-1 are strength-based
    std::vector<std::unique_ptr<wxCursor>> const mDownPlusCursors;
    std::vector<std::unique_ptr<wxCursor>> const mDownMinusCursors;
};

class SwirlTool final : public ContinuousTool
{
public:

    SwirlTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        ContinuousTool::Initialize(inputState);

        if (inputState.IsLeftMouseDown)
        {
            // Start sound
            mSoundController->PlaySwirlSound(mGameController->IsUnderwater(inputState.MousePosition));
        }

        SetBasisCursor(inputState);
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound
        mSoundController->StopSwirlSound();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        ContinuousTool::OnLeftMouseDown(inputState);

        // Start sound
        mSoundController->PlaySwirlSound(mGameController->IsUnderwater(inputState.MousePosition));

        // Change cursor
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Stop sound
        mSoundController->StopSwirlSound();

        // Change cursor
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyDown(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyUp(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
        ShowCurrentCursor();
    }

protected:

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) override;

private:

    void SetBasisCursor(InputState const & inputState)
    {
        if (inputState.IsLeftMouseDown)
        {
            if (inputState.IsShiftKeyDown)
            {
                // Down minus
                mCurrentCursor = mDownMinusCursors[0].get();
            }
            else
            {
                // Down plus
                mCurrentCursor = mDownPlusCursors[0].get();
            }
        }
        else
        {
            if (inputState.IsShiftKeyDown)
            {
                // Up minus
                mCurrentCursor = mUpMinusCursor.get();
            }
            else
            {
                // Up plus
                mCurrentCursor = mUpPlusCursor.get();
            }
        }
    }

    // The up cursors
    std::unique_ptr<wxCursor> const mUpPlusCursor;
    std::unique_ptr<wxCursor> const mUpMinusCursor;

    // The force-modulated down cursors;
    // cursor 0 is base; cursor 1 up to size-1 are strength-based
    std::vector<std::unique_ptr<wxCursor>> const mDownPlusCursors;
    std::vector<std::unique_ptr<wxCursor>> const mDownMinusCursors;
};

class PinTool final : public OneShotTool
{
public:

    PinTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        assert(!!mCursor);
        mCurrentCursor = mCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle pin
        mGameController->TogglePinAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    std::unique_ptr<wxCursor> const mCursor;
};

class TimerBombTool final : public OneShotTool
{
public:

    TimerBombTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        assert(!!mCursor);
        mCurrentCursor = mCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleTimerBombAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    std::unique_ptr<wxCursor> const mCursor;
};

class RCBombTool final : public OneShotTool
{
public:

    RCBombTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        assert(!!mCursor);
        mCurrentCursor = mCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleRCBombAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    std::unique_ptr<wxCursor> const mCursor;
};

class AntiMatterBombTool final : public OneShotTool
{
public:

    AntiMatterBombTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        assert(!!mCursor);
        mCurrentCursor = mCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleAntiMatterBombAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    std::unique_ptr<wxCursor> const mCursor;
};
