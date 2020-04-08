/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"

#include <Game/IGameController.h>
#include <Game/ResourceLoader.h>

#include <wx/image.h>
#include <wx/window.h>

#include <cassert>
#include <chrono>
#include <memory>
#include <optional>
#include <vector>

struct IToolCursorManager
{
    virtual void SetToolCursor(wxImage const & basisImage, float strength = 0.0f) = 0;
};

enum class ToolType
{
    Move = 0,
    MoveAll,
    PickAndPull,
    Smash,
    Saw,
    HeatBlaster,
    FireExtinguisher,
    Grab,
    Swirl,
    Pin,
    InjectAirBubbles,
    FloodHose,
    AntiMatterBomb,
    ImpactBomb,
    RCBomb,
    TimerBomb,
    WaveMaker,
    TerrainAdjust,
    Scrub,
    RepairStructure,
    ThanosSnap
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

protected:

    Tool(
        ToolType toolType,
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController)
        : mToolType(toolType)
        , mToolCursorManager(toolCursorManager)
        , mGameController(gameController)
        , mSoundController(soundController)
    {}

    IToolCursorManager & mToolCursorManager;
    std::shared_ptr<IGameController> const mGameController;
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

protected:

    OneShotTool(
        ToolType toolType,
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController)
        : Tool(
            toolType,
            toolCursorManager,
            std::move(gameController),
            std::move(soundController))
    {}
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

protected:

    ContinuousTool(
        ToolType toolType,
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController)
        : Tool(
            toolType,
            toolCursorManager,
            std::move(gameController),
            std::move(soundController))
        , mPreviousMousePosition()
        , mPreviousTimestamp(std::chrono::steady_clock::now())
        , mCumulatedTime(0)
    {}

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) = 0;

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
            // Tell IGameController to stop moving
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

            // Tell IGameController
            if (!mCurrentTrajectory->RotationCenter)
            {
                // Move
                mGameController->MoveBy(
                    mCurrentTrajectory->EngagedMovableObjectId,
                    newCurrentPosition - mCurrentTrajectory->CurrentPosition,
                    newCurrentPosition - mCurrentTrajectory->CurrentPosition);
            }
            else
            {
                // Rotate
                mGameController->RotateBy(
                    mCurrentTrajectory->EngagedMovableObjectId,
                    newCurrentPosition.y - mCurrentTrajectory->CurrentPosition.y,
                    *mCurrentTrajectory->RotationCenter,
                    newCurrentPosition.y - mCurrentTrajectory->CurrentPosition.y);
            }

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
                        // Move to stop
                        mGameController->MoveBy(
                            mCurrentTrajectory->EngagedMovableObjectId,
                            vec2f::zero(),
                            vec2f::zero());
                    }
                    else
                    {
                        // Rotate to stop
                        mGameController->RotateBy(
                            mCurrentTrajectory->EngagedMovableObjectId,
                            0.0f,
                            *mCurrentTrajectory->RotationCenter,
                            0.0f);
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

                // If we're enough into the trajectory, restart the timing - to avoid cramming a mouse move stretch
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
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        ProcessInputStateChange(inputState);
    }

    virtual void OnShiftKeyDown(InputState const & inputState) override
    {
        ProcessInputStateChange(inputState);
    }

    virtual void OnShiftKeyUp(InputState const & inputState) override
    {
        ProcessInputStateChange(inputState);
    }

protected:

    BaseMoveTool(
        ToolType toolType,
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        wxImage upCursorImage,
        wxImage downCursorImage,
        wxImage rotateUpCursorImage,
        wxImage rotateDownCursorImage)
        : OneShotTool(
            toolType,
            toolCursorManager,
            std::move(gameController),
            std::move(soundController))
        , mEngagedMovableObjectId()
        , mCurrentTrajectory()
        , mRotationCenter()
        , mUpCursorImage(upCursorImage)
        , mDownCursorImage(downCursorImage)
        , mRotateUpCursorImage(rotateUpCursorImage)
        , mRotateDownCursorImage(rotateDownCursorImage)
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

                    // Tell IGameController
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

                if (!!mCurrentTrajectory)
                {
                    //
                    // We are in the midst of a trajectory
                    //

                    // Impart last inertia == distance left
                    if (!mCurrentTrajectory->RotationCenter)
                    {
                        mGameController->MoveBy(
                            mCurrentTrajectory->EngagedMovableObjectId,
                            vec2f::zero(),
                            mCurrentTrajectory->EndPosition - mCurrentTrajectory->CurrentPosition);
                    }
                    else
                    {
                        // Rotate to stop
                        mGameController->RotateBy(
                            mCurrentTrajectory->EngagedMovableObjectId,
                            0.0f,
                            *mCurrentTrajectory->RotationCenter,
                            mCurrentTrajectory->EndPosition.y - mCurrentTrajectory->CurrentPosition.y);
                    }

                    // Stop trajectory
                    mCurrentTrajectory.reset();
                }

                // Tell IGameController we've stopped moving
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
                mToolCursorManager.SetToolCursor(mUpCursorImage);
            }
            else
            {
                mToolCursorManager.SetToolCursor(mRotateUpCursorImage);
            }
        }
        else
        {
            if (!mRotationCenter)
            {
                mToolCursorManager.SetToolCursor(mDownCursorImage);
            }
            else
            {
                mToolCursorManager.SetToolCursor(mRotateDownCursorImage);
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

        std::chrono::steady_clock::time_point StartTimestamp;
        std::chrono::steady_clock::time_point EndTimestamp;

        Trajectory(TMovableObjectId engagedMovableObjectId)
            : EngagedMovableObjectId(engagedMovableObjectId)
        {}
    };

    static constexpr std::chrono::milliseconds TrajectoryLag = std::chrono::milliseconds(500);

    // When set, we're smoothing the mouse position along a trajectory
    std::optional<Trajectory> mCurrentTrajectory;

    // When set, we're rotating
    std::optional<vec2f> mRotationCenter;

    // The cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;
    wxImage const mRotateUpCursorImage;
    wxImage const mRotateDownCursorImage;
};

class MoveTool final : public BaseMoveTool<ElementId>
{
public:

    MoveTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);
};

class MoveAllTool final : public BaseMoveTool<ShipId>
{
public:

    MoveAllTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);
};

class PickAndPullTool final : public Tool
{
public:

    PickAndPullTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        mCurrentEngagementState.reset();

        // Set cursor
        SetCurrentCursor();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
    }

    virtual void Update(InputState const & inputState) override
    {
        bool wasEngaged = !!mCurrentEngagementState;

        if (inputState.IsLeftMouseDown)
        {
            if (!mCurrentEngagementState)
            {
                //
                // Not engaged...
                // ...see if we're able to pick a point and thus start engagement
                //

                auto elementId = mGameController->PickObjectForPickAndPull(inputState.MousePosition);
                if (elementId.has_value())
                {
                    //
                    // Engage!
                    //

                    mCurrentEngagementState.emplace(
                        *elementId,
                        inputState.MousePosition);

                    // Play sound
                    mSoundController->PlayPliersSound(mGameController->IsUnderwater(*elementId));
                }
            }
            else
            {
                //
                // Engaged
                //

                // 1. Update target position
                mCurrentEngagementState->TargetScreenPosition = inputState.MousePosition;

                // 2. Converge towards target position
                mCurrentEngagementState->CurrentScreenPosition +=
                    (mCurrentEngagementState->TargetScreenPosition - mCurrentEngagementState->CurrentScreenPosition)
                    * 0.04f; // Convergence speed, magic number

                // 3. Apply force towards current position
                mGameController->Pull(
                    mCurrentEngagementState->PickedParticle,
                    mCurrentEngagementState->CurrentScreenPosition);
            }
        }
        else
        {
            // Disengage (in case we're engaged)
            mCurrentEngagementState.reset();
        }

        if (!!mCurrentEngagementState != wasEngaged)
        {
            // State change

            // Update cursor
            SetCurrentCursor();
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor()
    {
        mToolCursorManager.SetToolCursor(!!mCurrentEngagementState ? mDownCursorImage : mUpCursorImage);
    }

    // Our state

    struct EngagementState
    {
        ElementId PickedParticle;
        vec2f CurrentScreenPosition;
        vec2f TargetScreenPosition;

        EngagementState(
            ElementId pickedParticle,
            vec2f startScreenPosition)
            : PickedParticle(pickedParticle)
            , CurrentScreenPosition(startScreenPosition)
            , TargetScreenPosition(startScreenPosition)
        {}
    };

    std::optional<EngagementState> mCurrentEngagementState; // When set, indicates it's engaged

    // The cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;
};

class SmashTool final : public ContinuousTool
{
public:

    SmashTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        ContinuousTool::Initialize(inputState);

        // Reset cursor
        SetBasisCursor(inputState);
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        ContinuousTool::OnLeftMouseDown(inputState);

        // Reset cursor
        SetBasisCursor(inputState);
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Reset cursor
        SetBasisCursor(inputState);
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
            // Down
            mToolCursorManager.SetToolCursor(mDownCursorImage, 0.0f);
        }
        else
        {
            // Up
            mToolCursorManager.SetToolCursor(mUpCursorImage, 0.0f);
        }
    }

    wxImage mUpCursorImage;
    wxImage mDownCursorImage;
};

class SawTool final : public Tool
{
public:

    SawTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
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
        }
        else
        {
            // Reset state
            mPreviousMousePos = std::nullopt;
        }

        // Set cursor
        SetCurrentCursor(inputState);
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
            SetCurrentCursor(inputState);
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

        // Set cursor
        SetCurrentCursor(inputState);
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Reset state
        mPreviousMousePos = std::nullopt;

        // Stop sound
        mSoundController->StopSawSound();

        // Set cursor
        SetCurrentCursor(inputState);
    }

    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor(InputState const & inputState)
    {
        if (inputState.IsLeftMouseDown)
        {
            // Set current cursor to the current down cursor
            mToolCursorManager.SetToolCursor(mDownCursorCounter % 2 ? mDownCursorImage2 : mDownCursorImage1);
        }
        else
        {
            // Set current cursor to the up cursor
            mToolCursorManager.SetToolCursor(mUpCursorImage);
        }
    }

    // Our cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage1;
    wxImage const mDownCursorImage2;

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

class HeatBlasterTool final : public Tool
{
public:

    HeatBlasterTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        mCurrentAction = inputState.IsShiftKeyDown ? HeatBlasterActionType::Cool : HeatBlasterActionType::Heat;

        if (inputState.IsLeftMouseDown)
        {
            mIsEngaged = mGameController->ApplyHeatBlasterAt(inputState.MousePosition, mCurrentAction);
        }
        else
        {
            mIsEngaged = false;
        }

        SetCurrentCursor();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound
        mSoundController->StopHeatBlasterSound();
    }

    virtual void Update(InputState const & inputState) override
    {
        bool isEngaged;
        HeatBlasterActionType currentAction = inputState.IsShiftKeyDown ? HeatBlasterActionType::Cool : HeatBlasterActionType::Heat;

        if (inputState.IsLeftMouseDown)
        {
            isEngaged = mGameController->ApplyHeatBlasterAt(inputState.MousePosition, mCurrentAction);
        }
        else
        {
            isEngaged = false;
        }

        if (isEngaged)
        {
            if (!mIsEngaged
                || mCurrentAction != currentAction)
            {
                // State change
                mIsEngaged = true;
                mCurrentAction = currentAction;

                // Start sound
                mSoundController->PlayHeatBlasterSound(mCurrentAction);

                // Update cursor
                SetCurrentCursor();
            }
        }
        else
        {
            if (mIsEngaged)
            {
                // State change
                mIsEngaged = false;

                // Stop sound
                mSoundController->StopHeatBlasterSound();

                // Update cursor
                SetCurrentCursor();
            }
            else if (mCurrentAction != currentAction)
            {
                // State change
                mCurrentAction = currentAction;

                // Update cursor
                SetCurrentCursor();
            }
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor()
    {
        if (mIsEngaged)
        {
            mToolCursorManager.SetToolCursor(mCurrentAction == HeatBlasterActionType::Cool ? mCoolDownCursorImage : mHeatDownCursorImage);
        }
        else
        {
            mToolCursorManager.SetToolCursor(mCurrentAction == HeatBlasterActionType::Cool ? mCoolUpCursorImage : mHeatUpCursorImage);
        }
    }

    // Our state
    bool mIsEngaged;
    HeatBlasterActionType mCurrentAction;

    // The cursors
    wxImage const mHeatUpCursorImage;
    wxImage const mCoolUpCursorImage;
    wxImage const mHeatDownCursorImage;
    wxImage const mCoolDownCursorImage;
};

class FireExtinguisherTool final : public Tool
{
public:

    FireExtinguisherTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            mIsEngaged = mGameController->ExtinguishFireAt(inputState.MousePosition);
        }
        else
        {
            mIsEngaged = false;
        }

        SetCurrentCursor();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        // Stop sound
        mSoundController->StopFireExtinguisherSound();
    }

    virtual void Update(InputState const & inputState) override
    {
        bool isEngaged;

        if (inputState.IsLeftMouseDown)
        {
            isEngaged = mGameController->ExtinguishFireAt(inputState.MousePosition);
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
                mSoundController->PlayFireExtinguisherSound();

                // Update cursor
                SetCurrentCursor();
            }
        }
        else
        {
            if (mIsEngaged)
            {
                // State change
                mIsEngaged = false;

                // Stop sound
                mSoundController->StopFireExtinguisherSound();

                // Update cursor
                SetCurrentCursor();
            }
       }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor()
    {
        if (mIsEngaged)
        {
            mToolCursorManager.SetToolCursor(mDownCursorImage);
        }
        else
        {
            mToolCursorManager.SetToolCursor(mUpCursorImage);
        }
    }

    // Our state
    bool mIsEngaged;

    // The cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;
};

class GrabTool final : public ContinuousTool
{
public:

    GrabTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
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
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Stop sound
        mSoundController->StopDrawSound();

        // Change cursor
        SetBasisCursor(inputState);
    }

    virtual void OnShiftKeyDown(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
    }

    virtual void OnShiftKeyUp(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
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
                mToolCursorManager.SetToolCursor(mDownMinusCursorImage);
            }
            else
            {
                // Down plus
                mToolCursorManager.SetToolCursor(mDownPlusCursorImage);
            }
        }
        else
        {
            if (inputState.IsShiftKeyDown)
            {
                // Up minus
                mToolCursorManager.SetToolCursor(mUpMinusCursorImage);
            }
            else
            {
                // Up plus
                mToolCursorManager.SetToolCursor(mUpPlusCursorImage);
            }
        }
    }

    wxImage const mUpPlusCursorImage;
    wxImage const mUpMinusCursorImage;
    wxImage const mDownPlusCursorImage;
    wxImage const mDownMinusCursorImage;
};

class SwirlTool final : public ContinuousTool
{
public:

    SwirlTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
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
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Stop sound
        mSoundController->StopSwirlSound();

        // Change cursor
        SetBasisCursor(inputState);
    }

    virtual void OnShiftKeyDown(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
    }

    virtual void OnShiftKeyUp(InputState const & inputState) override
    {
        SetBasisCursor(inputState);
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
                mToolCursorManager.SetToolCursor(mDownMinusCursorImage);
            }
            else
            {
                // Down plus
                mToolCursorManager.SetToolCursor(mDownPlusCursorImage);
            }
        }
        else
        {
            if (inputState.IsShiftKeyDown)
            {
                // Up minus
                mToolCursorManager.SetToolCursor(mUpMinusCursorImage);
            }
            else
            {
                // Up plus
                mToolCursorManager.SetToolCursor(mUpPlusCursorImage);
            }
        }
    }

    wxImage const mUpPlusCursorImage;
    wxImage const mUpMinusCursorImage;
    wxImage const mDownPlusCursorImage;
    wxImage const mDownMinusCursorImage;
};

class PinTool final : public OneShotTool
{
public:

    PinTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        mToolCursorManager.SetToolCursor(mCursorImage);
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle pin
        mGameController->TogglePinAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    wxImage const mCursorImage;
};

class InjectAirBubblesTool final : public Tool
{
public:

    InjectAirBubblesTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
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

        SetCurrentCursor();
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
                SetCurrentCursor();
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
                SetCurrentCursor();
            }
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor()
    {
        mToolCursorManager.SetToolCursor(mIsEngaged ? mDownCursorImage : mUpCursorImage);
    }

    // Our state
    bool mIsEngaged;

    // The cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;
};

class FloodHoseTool final : public Tool
{
public:

    FloodHoseTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Update cursor
        SetCurrentCursor();
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
            SetCurrentCursor();
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
                SetCurrentCursor();
            }
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor()
    {
        mToolCursorManager.SetToolCursor(
            mIsEngaged
            ? ((mDownCursorCounter % 2) ? mDownCursorImage2 : mDownCursorImage1)
            : mUpCursorImage);
    }

    // Our state
    bool mIsEngaged;

    // The cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage1;
    wxImage const mDownCursorImage2;

    // The current counter for the down cursors
    uint8_t mDownCursorCounter;
};

class AntiMatterBombTool final : public OneShotTool
{
public:

    AntiMatterBombTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        mToolCursorManager.SetToolCursor(mCursorImage);
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleAntiMatterBombAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    wxImage const mCursorImage;
};

class ImpactBombTool final : public OneShotTool
{
public:

    ImpactBombTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        mToolCursorManager.SetToolCursor(mCursorImage);
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleImpactBombAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    wxImage const mCursorImage;
};

class RCBombTool final : public OneShotTool
{
public:

    RCBombTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        mToolCursorManager.SetToolCursor(mCursorImage);
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleRCBombAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    wxImage const mCursorImage;
};

class TimerBombTool final : public OneShotTool
{
public:

    TimerBombTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        mToolCursorManager.SetToolCursor(mCursorImage);
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleTimerBombAt(inputState.MousePosition);
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    wxImage const mCursorImage;
};

class WaveMakerTool final : public OneShotTool
{
public:

    WaveMakerTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            mGameController->AdjustOceanSurfaceTo(inputState.MousePosition);

            mSoundController->PlayWaveMakerSound();
        }

        // Set cursor
        SetCurrentCursor(inputState);
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

        // Set cursor
        SetCurrentCursor(inputState);
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        mSoundController->StopWaveMakerSound();

        mGameController->AdjustOceanSurfaceTo(std::nullopt);

        // Set cursor
        SetCurrentCursor(inputState);
    }

private:

    void SetCurrentCursor(InputState const & inputState)
    {
        if (inputState.IsLeftMouseDown)
        {
            mToolCursorManager.SetToolCursor(mDownCursorImage);
        }
        else
        {
            mToolCursorManager.SetToolCursor(mUpCursorImage);
        }
    }

    // The cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;
};

class TerrainAdjustTool final : public Tool
{
public:

    TerrainAdjustTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
            mCurrentTrajectoryPreviousPosition = inputState.MousePosition;
        else
            mCurrentTrajectoryPreviousPosition.reset();

        // Set cursor
        SetCurrentCursor();
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
            SetCurrentCursor();
        }
    }

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseDown(InputState const & /*inputState*/) override {}
    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor()
    {
        mToolCursorManager.SetToolCursor(!!mCurrentTrajectoryPreviousPosition ? mDownCursorImage : mUpCursorImage);
    }

    // Our state
    std::optional<vec2f> mCurrentTrajectoryPreviousPosition; // When set, indicates it's engaged

    // The cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;
};

class ScrubTool final : public Tool
{
public:

    ScrubTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            // Initialize state
            mPreviousMousePos = inputState.MousePosition;
        }
        else
        {
            // Reset state
            mPreviousMousePos = std::nullopt;
        }

        // Reset scrub detection
        mPreviousScrub.reset();
        mPreviousScrubTimestamp = std::chrono::steady_clock::time_point::min();

        // Set cursor
        SetCurrentCursor(inputState);
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
                            || abs(mPreviousScrub->angleCw(newScrub)) > Pi<float> / 2.0f    // Direction change
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

        // Set cursor
        SetCurrentCursor(inputState);
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Reset state
        mPreviousMousePos = std::nullopt;

        // Set cursor
        SetCurrentCursor(inputState);
    }

    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

private:

    void SetCurrentCursor(InputState const & inputState)
    {
        if (inputState.IsLeftMouseDown)
        {
            // Set current cursor to the down cursor
            mToolCursorManager.SetToolCursor(mDownCursorImage);
        }
        else
        {
            // Set current cursor to the up cursor
            mToolCursorManager.SetToolCursor(mUpCursorImage);
        }
    }

    // Our cursors
    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;

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
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset state
        mEngagementStartTimestamp.reset();

        // Update cursor
        UpdateCurrentCursor();
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

private:

    virtual void UpdateCurrentCursor()
    {
        wxImage cursorImage;

        if (!!mEngagementStartTimestamp)
        {
            auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - *mEngagementStartTimestamp);

            // Synchronize with sound
            auto cursorPhase = (totalElapsed.count() % 1000);
            if (cursorPhase < 87)
                cursorImage = mDownCursorImages[0]; // |
            else if (cursorPhase < 175)
                cursorImage = mDownCursorImages[1]; //
            else if (cursorPhase < 237)
                cursorImage = mDownCursorImages[2]; // /* \ */
            else if (cursorPhase < 300)
                cursorImage = mDownCursorImages[3]; //
            else if (cursorPhase < 500)
                cursorImage = mDownCursorImages[4]; // _
            else if (cursorPhase < 526)
                cursorImage = mDownCursorImages[3]; //
            else if (cursorPhase < 553)
                cursorImage = mDownCursorImages[2]; // /* \ */
            else if (cursorPhase < 580)
                cursorImage = mDownCursorImages[1]; //
            else
                cursorImage = mDownCursorImages[0]; // |
        }
        else
        {
            cursorImage = mUpCursorImage;
        }

        mToolCursorManager.SetToolCursor(cursorImage);
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
    wxImage const mUpCursorImage;
    std::array<wxImage, 5> const mDownCursorImages;
};

class ThanosSnapTool final : public OneShotTool
{
public:

    ThanosSnapTool(
        IToolCursorManager & toolCursorManager,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        // Reset cursor
        SetCurrentCursor(inputState);
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Do snap
        mGameController->ApplyThanosSnapAt(inputState.MousePosition);
        mSoundController->PlayThanosSnapSound();

        // Update cursor
        SetCurrentCursor(inputState);
    }

    virtual void OnLeftMouseUp(InputState const & inputState) override
    {
        // Update cursor
        SetCurrentCursor(inputState);
    }

private:

    void SetCurrentCursor(InputState const & inputState)
    {
        mToolCursorManager.SetToolCursor(inputState.IsLeftMouseDown ? mDownCursorImage : mUpCursorImage);
    }

    wxImage const mUpCursorImage;
    wxImage const mDownCursorImage;
};
