/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"

#include <Game/GameController.h>
#include <Game/ResourceLoader.h>

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
    MoveAll = 1,
    Smash = 2,
    Saw = 3,
    Grab = 4,
    Swirl = 5,
    Pin = 6,
    InjectAirBubbles = 7,
    FloodHose = 8,
    AntiMatterBomb = 9,
    ImpactBomb = 10,
    RCBomb = 11,
    TimerBomb = 12,
    WaveMaker = 13,
    TerrainAdjust = 14,
    Scrub = 15,
    RepairStructure = 16
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
        float strength)
    {
        // Calculate cursor index (cursor 0 is the base, we don't use it here)
        size_t const numberOfCursors = (cursors.size() - 1);
        size_t cursorIndex = 1u + static_cast<size_t>(
            floorf(
                strength * static_cast<float>(numberOfCursors - 1)));

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

template <typename TMovableObjectId>
class BaseMoveTool : public OneShotTool
{
public:

    virtual void Initialize(InputState const & inputState) override
    {
        // Reset state
        mEngagedMovableObjectId.reset();
        mCurrentTrajectory.reset();
        mRotationCenter.reset();

        // Initialize state
        ProcessInputStateChange(inputState);
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        if (!!mEngagedMovableObjectId)
        {
            // Tell GameController to stop moving
            mGameController->SetMoveToolEngaged(false);
        }
    }

    virtual void Update(InputState const & /*inputState*/) override
    {
        if (!!mCurrentTrajectory)
        {
            //
            // We're following a trajectory
            //

            auto const now = std::chrono::steady_clock::now();

            //
            // Smooth current position
            //

            float const rawProgress =
                static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - mCurrentTrajectory->StartTimestamp).count())
                / static_cast<float>(TrajectoryLag.count());

            // ((x+0.7)^2-0.49)/2.0
            float const progress = (pow(std::min(rawProgress, 1.0f) + 0.7f, 2.0f) - 0.49f) / 2.0f;

            vec2f const newCurrentPosition =
                mCurrentTrajectory->StartPosition
                + (mCurrentTrajectory->EndPosition - mCurrentTrajectory->StartPosition) * progress;

            // Tell GameController
            if (!mCurrentTrajectory->RotationCenter)
            {
                // Move
                mGameController->MoveBy(
                    mCurrentTrajectory->EngagedMovableObjectId,
                    newCurrentPosition - mCurrentTrajectory->CurrentPosition,
                    mCurrentTrajectory->PreviousVelocity);
            }
            else
            {
                // Rotate
                mGameController->RotateBy(
                    mCurrentTrajectory->EngagedMovableObjectId,
                    newCurrentPosition.y - mCurrentTrajectory->CurrentPosition.y,
                    *mCurrentTrajectory->RotationCenter,
                    mCurrentTrajectory->PreviousVelocity.y);
            }

            // Save this velocity as next step's previous velocity
            mCurrentTrajectory->PreviousVelocity = newCurrentPosition - mCurrentTrajectory->CurrentPosition;

            // Store new current position
            mCurrentTrajectory->CurrentPosition = newCurrentPosition;

            // Check whether we are done
            if (rawProgress >= 1.0f)
            {
                //
                // Close trajectory
                //

                if (!!mEngagedMovableObjectId)
                {
                    // Tell game controller to stop inertia
                    if (!mCurrentTrajectory->RotationCenter)
                    {
                        // Move
                        mGameController->MoveBy(
                            mCurrentTrajectory->EngagedMovableObjectId,
                            vec2f::zero(),
                            mCurrentTrajectory->PreviousVelocity);
                    }
                    else
                    {
                        // Rotate
                        mGameController->RotateBy(
                            mCurrentTrajectory->EngagedMovableObjectId,
                            0.0f,
                            *mCurrentTrajectory->RotationCenter,
                            mCurrentTrajectory->PreviousVelocity.y);
                    }
                }

                // Reset trajectory
                mCurrentTrajectory.reset();
            }
        }
    }

    virtual void OnMouseMove(InputState const & inputState) override
    {
        if (!!mEngagedMovableObjectId)
        {
            auto const now = std::chrono::steady_clock::now();

            if (!!mCurrentTrajectory)
            {
                //
                // We already have a trajectory
                //

                // Re-register the start position from where we are now
                mCurrentTrajectory->StartPosition = mCurrentTrajectory->CurrentPosition;

                // If we're enough into the trajectory, restart the timing - to avoid cramming a mouse move stretche
                // into a tiny little time interval
                if (now > mCurrentTrajectory->StartTimestamp + TrajectoryLag / 2)
                {
                    mCurrentTrajectory->StartTimestamp = now;
                    mCurrentTrajectory->EndTimestamp = mCurrentTrajectory->StartTimestamp + TrajectoryLag;
                }
            }
            else
            {
                //
                // Start a new trajectory
                //

                mCurrentTrajectory = Trajectory(*mEngagedMovableObjectId);
                mCurrentTrajectory->RotationCenter = mRotationCenter;
                mCurrentTrajectory->StartPosition = inputState.PreviousMousePosition;
                mCurrentTrajectory->CurrentPosition = mCurrentTrajectory->StartPosition;
                mCurrentTrajectory->StartTimestamp = now;
                mCurrentTrajectory->EndTimestamp = mCurrentTrajectory->StartTimestamp + TrajectoryLag;
            }

            mCurrentTrajectory->EndPosition = inputState.MousePosition;
        }
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        ProcessInputStateChange(inputState);
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        ProcessInputStateChange(inputState);
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyDown(InputState const & inputState) override
    {
        ProcessInputStateChange(inputState);
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyUp(InputState const & inputState) override
    {
        ProcessInputStateChange(inputState);
        ShowCurrentCursor();
    }

protected:

    BaseMoveTool(
        ToolType toolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        std::unique_ptr<wxCursor> upCursor,
        std::unique_ptr<wxCursor> downCursor,
        std::unique_ptr<wxCursor> rotateUpCursor,
        std::unique_ptr<wxCursor> rotateDownCursor)
        : OneShotTool(
            toolType,
            parentFrame,
            std::move(gameController),
            std::move(soundController))
        , mEngagedMovableObjectId()
        , mCurrentTrajectory()
        , mRotationCenter()
        , mUpCursor(std::move(upCursor))
        , mDownCursor(std::move(downCursor))
        , mRotateUpCursor(std::move(rotateUpCursor))
        , mRotateDownCursor(std::move(rotateDownCursor))
    {
    }

private:

    void ProcessInputStateChange(InputState const & inputState)
    {
        //
        // Update state
        //

        if (inputState.IsLeftMouseDown)
        {
            // Left mouse down

            if (!mEngagedMovableObjectId)
            {
                //
                // We're currently not engaged
                //

                // Check with game controller
                mGameController->PickObjectToMove(
                    inputState.MousePosition,
                    mEngagedMovableObjectId);

                if (!!mEngagedMovableObjectId)
                {
                    // Engaged!

                    // Tell GameController
                    mGameController->SetMoveToolEngaged(true);
                }
            }
        }
        else
        {
            // Left mouse up

            if (!!mEngagedMovableObjectId)
            {
                //
                // We're currently engaged
                //

                // Disengage
                mEngagedMovableObjectId.reset();

                // Reset rotation
                mRotationCenter.reset();

                // Stop trajectory
                mCurrentTrajectory.reset();

                // Tell GameController we've stopped moving
                mGameController->SetMoveToolEngaged(false);
            }
        }

        if (inputState.IsShiftKeyDown)
        {
            // Shift key down

            if (!mRotationCenter && !!mEngagedMovableObjectId)
            {
                //
                // We're engaged and not in rotation mode yet
                //

                assert(inputState.IsLeftMouseDown);

                // Start rotation mode
                mRotationCenter = inputState.MousePosition;
            }
        }
        else
        {
            // Shift key up

            if (!!mRotationCenter)
            {
                //
                // We are in rotation mode
                //

                // Stop rotation mode
                mRotationCenter.reset();
            }
        }

        //
        // Update cursor
        //

        if (!mEngagedMovableObjectId)
        {
            if (!mRotationCenter)
            {
                mCurrentCursor = mUpCursor.get();
            }
            else
            {
                mCurrentCursor = mRotateUpCursor.get();
            }
        }
        else
        {
            if (!mRotationCenter)
            {
                mCurrentCursor = mDownCursor.get();
            }
            else
            {
                mCurrentCursor = mRotateDownCursor.get();
            }
        }
    }

private:

    // When engaged, the ID of the element (point of a ship) we're currently moving
    std::optional<TMovableObjectId> mEngagedMovableObjectId;

    struct Trajectory
    {
        TMovableObjectId EngagedMovableObjectId;
        std::optional<vec2f> RotationCenter;

        vec2f StartPosition;
        vec2f CurrentPosition;
        vec2f EndPosition;

        vec2f PreviousVelocity;

        std::chrono::steady_clock::time_point StartTimestamp;
        std::chrono::steady_clock::time_point EndTimestamp;

        Trajectory(TMovableObjectId engagedMovableObjectId)
            : EngagedMovableObjectId(engagedMovableObjectId)
        {}
    };

    static constexpr std::chrono::milliseconds TrajectoryLag = std::chrono::milliseconds(300);

    // When set, we're smoothing the mouse position along a trajectory
    std::optional<Trajectory> mCurrentTrajectory;

    // When set, we're rotating
    std::optional<vec2f> mRotationCenter;

    // The cursors
    std::unique_ptr<wxCursor> const mUpCursor;
    std::unique_ptr<wxCursor> const mDownCursor;
    std::unique_ptr<wxCursor> const mRotateUpCursor;
    std::unique_ptr<wxCursor> const mRotateDownCursor;
};

class MoveTool final : public BaseMoveTool<ElementId>
{
public:

    MoveTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);
};

class MoveAllTool final : public BaseMoveTool<ShipId>
{
public:

    MoveAllTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);
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

class InjectAirBubblesTool final : public Tool
{
public:

    InjectAirBubblesTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            mIsEngaged = mGameController->InjectBubblesAt(inputState.MousePosition);
        }
        else
        {
            mIsEngaged = false;
        }
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound
        mSoundController->StopAirBubblesSound();
    }

    virtual void Update(InputState const & inputState) override
    {
        bool isEngaged;
        if (inputState.IsLeftMouseDown)
        {
            isEngaged = mGameController->InjectBubblesAt(inputState.MousePosition);
        }
        else
        {
            isEngaged = false;
        }

        if (isEngaged)
        {
            if (!mIsEngaged)
            {
                // State change
                mIsEngaged = true;

                // Start sound
                mSoundController->PlayAirBubblesSound();

                // Update cursor
                ShowCurrentCursor();
            }
        }
        else
        {
            if (mIsEngaged)
            {
                // State change
                mIsEngaged = false;

                // Stop sound
                mSoundController->StopAirBubblesSound();

                // Update cursor
                ShowCurrentCursor();
            }
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);

        mParentFrame->SetCursor(mIsEngaged ? *mDownCursor : *mUpCursor);
    }

private:

    // Our state
    bool mIsEngaged;

    // The cursors
    std::unique_ptr<wxCursor> const mUpCursor;
    std::unique_ptr<wxCursor> const mDownCursor;
};

class FloodHoseTool final : public Tool
{
public:

    FloodHoseTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound
        mSoundController->StopFloodHoseSound();
    }

    virtual void Update(InputState const & inputState) override
    {
        bool isEngaged;
        if (inputState.IsLeftMouseDown)
        {
            isEngaged = mGameController->FloodAt(
                inputState.MousePosition,
                inputState.IsShiftKeyDown ? -1.0f : 1.0f);
        }
        else
        {
            isEngaged = false;
        }

        if (isEngaged)
        {
            if (!mIsEngaged)
            {
                // State change
                mIsEngaged = true;

                // Start sound
                mSoundController->PlayFloodHoseSound();
            }
            else
            {
                // Update down cursor
                ++mDownCursorCounter;
            }

            // Update cursor
            ShowCurrentCursor();
        }
        else
        {
            if (mIsEngaged)
            {
                // State change
                mIsEngaged = false;

                // Stop sound
                mSoundController->StopFloodHoseSound();

                // Update cursor
                ShowCurrentCursor();
            }
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);

        mParentFrame->SetCursor(
            mIsEngaged
            ? ((mDownCursorCounter % 2) ? *mDownCursor2 : *mDownCursor1)
            : *mUpCursor);
    }

private:

    // Our state
    bool mIsEngaged;

    // The cursors
    std::unique_ptr<wxCursor> const mUpCursor;
    std::unique_ptr<wxCursor> const mDownCursor1;
    std::unique_ptr<wxCursor> const mDownCursor2;

    // The current counter for the down cursors
    uint8_t mDownCursorCounter;
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

class ImpactBombTool final : public OneShotTool
{
public:

    ImpactBombTool(
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
        mGameController->ToggleImpactBombAt(inputState.MousePosition);
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

class WaveMakerTool final : public OneShotTool
{
public:

    WaveMakerTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            mGameController->AdjustOceanSurfaceTo(inputState.MousePosition);

            mSoundController->PlayWaveMakerSound();

            mCurrentCursor = mDownCursor.get();
        }
        else
        {
            mCurrentCursor = mUpCursor.get();
        }

        ShowCurrentCursor();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        mSoundController->StopWaveMakerSound();
    }

    virtual void OnMouseMove(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            mGameController->AdjustOceanSurfaceTo(inputState.MousePosition);
        }
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        mGameController->AdjustOceanSurfaceTo(inputState.MousePosition);

        mSoundController->PlayWaveMakerSound();

        mCurrentCursor = mDownCursor.get();
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override
    {
        mSoundController->StopWaveMakerSound();

        mGameController->AdjustOceanSurfaceTo(std::nullopt);

        mCurrentCursor = mUpCursor.get();
        ShowCurrentCursor();
    }

private:

    // The cursors
    std::unique_ptr<wxCursor> const mUpCursor;
    std::unique_ptr<wxCursor> const mDownCursor;
};

class TerrainAdjustTool final : public Tool
{
public:

    TerrainAdjustTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
            mCurrentTrajectoryPreviousPosition = inputState.MousePosition;
        else
            mCurrentTrajectoryPreviousPosition.reset();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
    }

    virtual void Update(InputState const & inputState) override
    {
        bool wasEngaged = !!mCurrentTrajectoryPreviousPosition;

        if (inputState.IsLeftMouseDown)
        {
            if (!mCurrentTrajectoryPreviousPosition)
                mCurrentTrajectoryPreviousPosition = inputState.MousePosition;

            bool isAdjusted = mGameController->AdjustOceanFloorTo(
                *mCurrentTrajectoryPreviousPosition,
                inputState.MousePosition);

            if (isAdjusted)
            {
                mSoundController->PlayTerrainAdjustSound();
            }

            mCurrentTrajectoryPreviousPosition = inputState.MousePosition;
        }
        else
        {
            mCurrentTrajectoryPreviousPosition.reset();
        }

        if (!!mCurrentTrajectoryPreviousPosition != wasEngaged)
        {
            // State change

            // Update cursor
            ShowCurrentCursor();
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);

        mParentFrame->SetCursor(!!mCurrentTrajectoryPreviousPosition ? *mDownCursor : *mUpCursor);
    }

private:

    // Our state
    std::optional<vec2f> mCurrentTrajectoryPreviousPosition; // When set, indicates it's engaged

    // The cursors
    std::unique_ptr<wxCursor> const mUpCursor;
    std::unique_ptr<wxCursor> const mDownCursor;
};

class ScrubTool final : public Tool
{
public:

    ScrubTool(
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

            // Set current cursor to the down cursor
            mCurrentCursor = mDownCursor.get();
        }
        else
        {
            // Reset state
            mPreviousMousePos = std::nullopt;

            // Set current cursor to the up cursor
            mCurrentCursor = mUpCursor.get();
        }

        // Reset scrub detection
        mPreviousScrub.reset();
        mPreviousScrubTimestamp = std::chrono::steady_clock::time_point::min();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override {}

    virtual void Update(InputState const & /*inputState*/) override {}

    virtual void OnMouseMove(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            if (!!mPreviousMousePos)
            {
                // Do a scrub strike
                bool hasScrubbed = mGameController->ScrubThrough(
                    *mPreviousMousePos,
                    inputState.MousePosition);

                // See if we should emit a sound
                if (hasScrubbed)
                {
                    vec2f const newScrub = inputState.MousePosition - *mPreviousMousePos;
                    if (newScrub.length() > 1.0f)
                    {
                        auto const now = std::chrono::steady_clock::now();

                        if (!mPreviousScrub
                            || abs(mPreviousScrub->angle(newScrub)) > Pi<float> / 2.0f    // Direction change
                            || (now - mPreviousScrubTimestamp) > std::chrono::milliseconds(250))
                        {
                            // Play sound
                            mSoundController->PlayScrubSound();

                            mPreviousScrub = newScrub;
                            mPreviousScrubTimestamp = now;
                        }
                    }
                }
            }

            // Remember the next previous mouse position
            mPreviousMousePos = inputState.MousePosition;
        }
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Initialize state
        mPreviousMousePos = inputState.MousePosition;
        mPreviousScrub.reset();
        mPreviousScrubTimestamp = std::chrono::steady_clock::time_point::min();

        // Set current cursor to the down cursor
        mCurrentCursor = mDownCursor.get();
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override
    {
        // Reset state
        mPreviousMousePos = std::nullopt;

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
    std::unique_ptr<wxCursor> const mDownCursor;

    // The currently-selected cursor that will be shown
    wxCursor * mCurrentCursor;

    //
    // State
    //

    // The previous mouse position; when set, we have a segment and can scrub
    std::optional<vec2f> mPreviousMousePos;

    // The previous scrub vector, which we want to remember in order to
    // detect directions changes for the scrubbing sound
    std::optional<vec2f> mPreviousScrub;

    // The time at which we have last played a scrub sound
    std::chrono::steady_clock::time_point mPreviousScrubTimestamp;
};

class RepairStructureTool final : public Tool
{
public:

    RepairStructureTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Just set a cursor, we'll update it at Update()
        mCurrentCursor = mUpCursor.get();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound, just in case
        mSoundController->StopRepairStructureSound();
    }

    virtual void Update(InputState const & inputState) override
    {
        bool isEngaged;
        if (inputState.IsLeftMouseDown)
        {
            isEngaged = true;
        }
        else
        {
            isEngaged = false;
        }

        if (isEngaged)
        {
            if (!mEngagementStartTimestamp)
            {
                // State change
                mCurrentSessionId += 1;
                mCurrentSessionStepId = 0;
                mEngagementStartTimestamp = std::chrono::steady_clock::now();

                // Start sound
                mSoundController->PlayRepairStructureSound();
            }
            else
            {
                // Increment step
                mCurrentSessionStepId += 1;
            }

            // Repair
            mGameController->RepairAt(
                inputState.MousePosition,
                1.0f,
                mCurrentSessionId,
                mCurrentSessionStepId);

            // Update cursor
            UpdateCurrentCursor();
        }
        else
        {
            if (!!mEngagementStartTimestamp)
            {
                // State change
                mEngagementStartTimestamp.reset();

                // Stop sound
                mSoundController->StopRepairStructureSound();

                // Update cursor
                UpdateCurrentCursor();
            }
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);

        mParentFrame->SetCursor(*mCurrentCursor);
    }

private:

    virtual void UpdateCurrentCursor()
    {
        if (!!mEngagementStartTimestamp)
        {
            auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - *mEngagementStartTimestamp);

            // Synchronize with sound
            auto cursorPhase = (totalElapsed.count() % 1000);
            if (cursorPhase < 87)
                mCurrentCursor = mDownCursors[0].get(); // |
            else if (cursorPhase < 175)
                mCurrentCursor = mDownCursors[1].get(); //
            else if (cursorPhase < 237)
                mCurrentCursor = mDownCursors[2].get(); // /* \ */
            else if (cursorPhase < 300)
                mCurrentCursor = mDownCursors[3].get(); //
            else if (cursorPhase < 500)
                mCurrentCursor = mDownCursors[4].get(); // _
            else if (cursorPhase < 526)
                mCurrentCursor = mDownCursors[3].get(); //
            else if (cursorPhase < 553)
                mCurrentCursor = mDownCursors[2].get(); // /* \ */
            else if (cursorPhase < 580)
                mCurrentCursor = mDownCursors[1].get(); //
            else
                mCurrentCursor = mDownCursors[0].get(); // |
        }
        else
        {
            mCurrentCursor = mUpCursor.get();
        }

        ShowCurrentCursor();
    }

private:

    // When set, we are engaged
    std::optional<std::chrono::steady_clock::time_point> mEngagementStartTimestamp;

    // The current session id and the current step id in the session
    RepairSessionId mCurrentSessionId;
    RepairSessionStepId mCurrentSessionStepId;

    // The currently-chosen cursor
    wxCursor * mCurrentCursor;

    // Our cursors
    std::unique_ptr<wxCursor> const mUpCursor;
    std::array<std::unique_ptr<wxCursor>, 5> const mDownCursors;

};