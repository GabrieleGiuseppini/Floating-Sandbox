/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"
#include "Tools.h"

#include <GameLib/GameController.h>
#include <GameLib/ResourceLoader.h>

#include <wx/frame.h>

#include <memory>
#include <vector>

class ToolController
{
public:

    ToolController(
        ToolType initialToolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader & resourceLoader);

    void SetTool(ToolType toolType)
    {
        assert(static_cast<size_t>(toolType) < mAllTools.size());

        // Notify old tool
        if (nullptr != mCurrentTool)
            mCurrentTool->Deinitialize(mInputState);

        // Switch tool
        mCurrentTool = mAllTools[static_cast<size_t>(toolType)].get();
        mCurrentTool->Initialize(mInputState);

        // Show its cursor
        ShowCurrentCursor();
    }

    void UnsetTool()
    {
        // Notify old tool
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->Deinitialize(mInputState);
            mCurrentTool->ShowCurrentCursor();
        }
    }

    void ShowCurrentCursor()
    {
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->ShowCurrentCursor();
        }
    }

    void Update()
    {
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->Update(mInputState);
        }
    }

    //
    // Getters
    //

    vec2f const & GetMouseScreenCoordinates() const
    {
        return mInputState.MousePosition;
    }

    //
    // External event handlers
    //

    void OnMouseMove(
        int x,
        int y);

    void OnLeftMouseDown();

    void OnLeftMouseUp();

    void OnRightMouseDown();

    void OnRightMouseUp();

    void OnShiftKeyDown();

    void OnShiftKeyUp();

private:

    // Input state
    InputState mInputState;

    // Tool state
    Tool * mCurrentTool;
    std::vector<std::unique_ptr<Tool>> mAllTools; // Indexed by enum

private:

    wxFrame * const mParentFrame;
    std::unique_ptr<wxCursor> mPanCursor;
    std::shared_ptr<GameController> const mGameController;
    std::shared_ptr<SoundController> const mSoundController;
};
