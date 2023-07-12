/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"
#include "Tools.h"

#include <Game/IGameController.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/frame.h>
#include <wx/image.h>

#include <cassert>
#include <memory>
#include <vector>

class ToolController final
    : public IToolCursorManager
{
public:

    ToolController(
        ToolType initialToolType,
        float initialEffectiveAmbientLightIntensity,
        wxWindow * parentWindow,
        IGameController & gameController,
        SoundController & soundController,
        ResourceLocator const & resourceLocator);

    void SetTool(ToolType toolType)
    {
        assert(static_cast<size_t>(toolType) < mAllTools.size());

        // Notify old tool
        if (nullptr != mCurrentTool)
            mCurrentTool->Deinitialize();

        // Switch tool
        mCurrentTool = mAllTools[static_cast<size_t>(toolType)].get();
        mCurrentTool->Initialize(mInputState);

        // Show its cursor
        InternalSetCurrentToolCursor();
    }

    void SetHumanNpcPlaceTool(HumanNpcRoleType role)
    {
        PlaceHumanNpcTool * tool = dynamic_cast<PlaceHumanNpcTool *>(mAllTools[static_cast<size_t>(ToolType::PlaceHumanNpc)].get());
        tool->SetRole(role);
        SetTool(ToolType::PlaceHumanNpc);
    }

    void UnsetTool()
    {
        // Notify old tool
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->Deinitialize();
            InternalSetCurrentToolCursor();
        }
    }

    void UpdateSimulation(float currentSimulationTime)
    {
        // See if cursor brightness has changed
        float const newCurrentToolCursorBrightness = CalculateCursorBrightness(mGameController.GetEffectiveAmbientLightIntensity());
        if (newCurrentToolCursorBrightness != mCurrentToolCursorBrightness)
        {
            mCurrentToolCursorBrightness = newCurrentToolCursorBrightness;
            InternalSetCurrentToolCursor();
        }

        // Update tools
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->UpdateSimulation(mInputState, currentSimulationTime);
        }
    }

    void Reset()
    {
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->Deinitialize();
            mCurrentTool->Initialize(mInputState);

            InternalSetCurrentToolCursor();
        }
    }

    //
    // Getters
    //

    DisplayLogicalCoordinates const & GetMouseScreenCoordinates() const
    {
        return mInputState.MousePosition;
    }

    //
    // External event handlers
    //

    void OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition);

    void OnLeftMouseDown();

    void OnLeftMouseUp();

    void OnRightMouseDown();

    void OnRightMouseUp();

    void OnShiftKeyDown();

    void OnShiftKeyUp();

    //
    // IToolCursorHandler
    //

    virtual void SetToolCursor(wxImage const & basisImage, float strength = 0.0f) override;

private:

    static float CalculateCursorBrightness(float effectiveAmbientLightIntensity);

    void InternalSetCurrentToolCursor();

private:

    // Input state
    InputState mInputState;

    // Tool state
    Tool * mCurrentTool;
    std::vector<std::unique_ptr<Tool>> mAllTools; // Indexed by enum

private:

    wxWindow * const mParentWindow;
    wxCursor mPanCursor;
    IGameController & mGameController;
    SoundController & mSoundController;

private:

    //
    // Cursor
    //

    struct ToolCursor
    {
        wxImage BasisImage;
        float Strength;

        ToolCursor()
            : BasisImage()
            , Strength(0.0f)
        {}

        ToolCursor(
            wxImage const & basisImage,
            float strength)
            : BasisImage(basisImage)
            , Strength(strength)
        {}
    };

    ToolCursor mCurrentToolCursor;
    float mCurrentToolCursorBrightness;
};
