/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <UILib/LocalizationManager.h>
#include <UILib/LoggingDialog.h>

#include <Game/ResourceLocator.h>

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/panel.h>

namespace ShipBuilder {

/*
 * The main window of the ship builder GUI.
 */
class MainFrame final : public wxFrame
{
public:

    MainFrame(
        wxApp * mainApp,
        ResourceLocator const & resourceLocator,
        LocalizationManager const & localizationManager);

    void Open();

private:

    wxPanel * CreateFilePanel();
    wxPanel * CreateToolSettingsPanel();
    wxPanel * CreateGamePanel();
    wxPanel * CreateViewPanel();
    wxPanel * CreateToolbarPanel();
    wxPanel * CreateWorkPanel();

    void OnWorkCanvasResize(wxSizeEvent & event);
    void OnWorkCanvasLeftDown(wxMouseEvent & event);
    void OnWorkCanvasLeftUp(wxMouseEvent & event);
    void OnWorkCanvasRightDown(wxMouseEvent & event);
    void OnWorkCanvasRightUp(wxMouseEvent & event);
    void OnWorkCanvasMouseMove(wxMouseEvent & event);
    void OnWorkCanvasMouseWheel(wxMouseEvent & event);
    void OnQuit(wxCommandEvent & event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent & event);

private:

    wxApp * const mMainApp;

    //
    // Helpers
    //

    ResourceLocator const & mResourceLocator;
    LocalizationManager const & mLocalizationManager;

    //
    // UI
    //

    wxPanel * mMainPanel;
    wxWindow * mWorkCanvas;
    bool mIsMouseCapturedByWorkCanvas;

    //
    // Dialogs
    //

    std::unique_ptr<LoggingDialog> mLoggingDialog;
};

}