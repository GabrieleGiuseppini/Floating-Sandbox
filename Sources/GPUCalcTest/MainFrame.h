/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-05
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

// Bring-in our glad GL environment, before any of the
// WX OpenGL includes redefines it
#include <OpenGLCore/GameOpenGL.h>

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/*
 * The main window of the test GUI.
 */
class MainFrame
    : public wxFrame
{
public:

    MainFrame(wxApp * mainApp);

    virtual ~MainFrame();

private:

    wxBoxSizer * mMainFrameSizer;

private:

    //
    // Event handlers
    //

    // App
    void OnMainFrameClose(wxCloseEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);

private:

    void OnError(
        std::string const & message,
        bool die);

    void OnLogMessage(std::string const & message);
    void ClearLog();

    void RunOpenGLTest();
    void RunPixelCoordsTest(size_t dataPoints);
    void RunAddTest(size_t dataPoints);
    void RunAllTests();

private:

    wxApp * const mMainApp;
    GameAssetManager mGameAssetManager;

    wxTextCtrl * mLogTextCtrl;

private:

    std::unique_ptr<wxGLCanvas> mDummyGLCanvas;
    std::unique_ptr<wxGLContext> mDummyGLContext;
};
