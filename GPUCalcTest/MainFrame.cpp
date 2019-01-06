/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-05
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "OpenGLContext.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>
#include <GameCore/ResourceLoader.h>
#include <GameCore/Utils.h>

#include <GPUCalc/GPUCalculatorFactory.h>

#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/string.h>

#include <cassert>

const long ID_MAIN_CANVAS = wxNewId();

MainFrame::MainFrame(wxApp * mainApp)
    : mMainApp(mainApp)
{
    Create(
        nullptr,
        wxID_ANY,
        "GPUCalc Test",
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_FRAME_STYLE,
        _T("Main Frame"));

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    Connect(this->GetId(), wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&MainFrame::OnMainFrameClose);
    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnPaint);

    mMainFrameSizer = new wxBoxSizer(wxVERTICAL);
    // ...
    SetSizerAndFit(mMainFrameSizer);

    try
    {
        //
        // Create Test GPUCalculator
        //

        GPUCalculatorFactory::GetInstance().Initialize(
            [this]() -> std::unique_ptr<IOpenGLContext>
            {
                return std::make_unique<OpenGLContext>();
            },
            ResourceLoader::GetGPUCalcShadersRootPath());

        mTestGPUCalculator = GPUCalculatorFactory::GetInstance().CreateTestCalculator(5);


        //
        // Do test
        //

        vec2f a[5] = {
            {1.0f, 1.0f},
            {2.0f, 1.0f},
            {3.0f, 1.0f},
            {4.0f, 1.0f},
            {5.0f, 1.0f}
        };

        vec2f b[5] = {
            {0.1f, 10.0f},
            {0.2f, 20.0f},
            {0.4f, 30.0f},
            {0.8f, 40.0f},
            {1.0f, 50.0f}
        };

        vec2f result[5] = {
            {0.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f}
        };

        mTestGPUCalculator->Add(a, b, result);

        // TODOTEST
        mTestGPUCalculator2 = GPUCalculatorFactory::GetInstance().CreateTestCalculator(5);
        mTestGPUCalculator2->Add(a, b, result);

        // TODOHERE: verify
    }
    catch (std::exception const & ex)
    {
        OnError(ex.what(), true);
    }
}

MainFrame::~MainFrame()
{
}

//
// App event handlers
//

void MainFrame::OnMainFrameClose(wxCloseEvent & /*event*/)
{
    Destroy();
}

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
    Close();
}

void MainFrame::OnPaint(wxPaintEvent & event)
{
    // This happens sparingly, mostly when the window is resized and when it's shown

    event.Skip();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void MainFrame::OnError(
    std::string const & message,
    bool die)
{
    //
    // Show message
    //

    wxMessageBox(message, wxT("Maritime Disaster"), wxICON_ERROR);

    if (die)
    {
        //
        // Exit
        //

        this->Destroy();
    }
}