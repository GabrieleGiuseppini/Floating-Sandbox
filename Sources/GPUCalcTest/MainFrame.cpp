/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-05
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "OpenGLContext.h"
#include "TestRun.h"

#include "AddTest.h"
#include "OpenGLInitTest.h"
#include "PixelCoordsTest.h"

#include <GPUCalc/GPUCalculatorFactory.h>

#include <Core/GameException.h>
#include <Core/Log.h>

#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/colour.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/string.h>

#include <cassert>

const long ID_MAIN_CANVAS = wxNewId();

MainFrame::MainFrame(wxApp * mainApp)
    : mMainApp(mainApp)
{
    (void)mMainApp; // Keeping it around

    Create(
        nullptr,
        wxID_ANY,
        "GPUCalc Test",
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_FRAME_STYLE);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    Connect(this->GetId(), wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&MainFrame::OnMainFrameClose);
    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnPaint);

    mMainFrameSizer = new wxBoxSizer(wxHORIZONTAL);

    //
    // Tests
    //

    auto buttonCol1Sizer = new wxBoxSizer(wxVERTICAL);

    auto initOpenglButton = new wxButton(this, wxID_ANY, "Init OpenGL");
    initOpenglButton->SetMaxSize(wxSize(-1, 20));
    initOpenglButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            this->RunOpenGLTest();
        });
    buttonCol1Sizer->Add(initOpenglButton, 1, wxEXPAND);

    auto pixelCoords5TestButton = new wxButton(this, wxID_ANY, "Run PixelCoords(5) Test");
    pixelCoords5TestButton->SetMaxSize(wxSize(-1, 20));
    pixelCoords5TestButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            this->RunPixelCoordsTest(5);
        });
    buttonCol1Sizer->Add(pixelCoords5TestButton, 1, wxEXPAND);

    auto pixelCoords65536TestButton = new wxButton(this, wxID_ANY, "Run PixelCoords(65536) Test");
    pixelCoords65536TestButton->SetMaxSize(wxSize(-1, 20));
    pixelCoords65536TestButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            this->RunPixelCoordsTest(65536);
        });
    buttonCol1Sizer->Add(pixelCoords65536TestButton, 1, wxEXPAND);

    auto Add5TestButton = new wxButton(this, wxID_ANY, "Run Add(5) Test");
    Add5TestButton->SetMaxSize(wxSize(-1, 20));
    Add5TestButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            this->RunAddTest(5);
        });
    buttonCol1Sizer->Add(Add5TestButton, 1, wxEXPAND);

    auto Add65536TestButton = new wxButton(this, wxID_ANY, "Run Add(65536) Test");
    Add65536TestButton->SetMaxSize(wxSize(-1, 20));
    Add65536TestButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            this->RunAddTest(65536);
        });
    buttonCol1Sizer->Add(Add65536TestButton, 1, wxEXPAND);

    auto allTestsButton = new wxButton(this, wxID_ANY, "Run All Tests");
    allTestsButton->SetMaxSize(wxSize(-1, 20));
    allTestsButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            this->RunAllTests();
        });
    buttonCol1Sizer->Add(allTestsButton, 1, wxEXPAND);


    mMainFrameSizer->Add(buttonCol1Sizer, 1, wxEXPAND);


    //
    // Log
    //

    auto logSizer = new wxBoxSizer(wxVERTICAL);

    mLogTextCtrl = new wxTextCtrl(
        this,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(-1, -1),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxVSCROLL | wxHSCROLL);

    wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    mLogTextCtrl->SetFont(font);

    logSizer->Add(mLogTextCtrl, 1, wxEXPAND);


    auto logButtonsSizer = new wxBoxSizer(wxHORIZONTAL);

    auto clearLogButton = new wxButton(this, wxID_ANY, "Clear Log");
    clearLogButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            this->ClearLog();
        });
    logButtonsSizer->Add(clearLogButton, 0);

    auto copyLogButton = new wxButton(this, wxID_ANY, "Copy Log");
    copyLogButton->Bind(
        wxEVT_BUTTON,
        [this](wxEvent & /*event*/)
        {
            if (wxTheClipboard->Open())
            {
                wxTheClipboard->Clear();
                wxTheClipboard->SetData(new wxTextDataObject(this->mLogTextCtrl->GetValue()));
                wxTheClipboard->Flush();
                wxTheClipboard->Close();
            }
        });
    logButtonsSizer->Add(copyLogButton, 0);

    logSizer->Add(logButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);

    mMainFrameSizer->Add(logSizer, 9, wxEXPAND);


    // Finalize frame
    SetSizerAndFit(mMainFrameSizer);


    // Register log listener
    Logger::Instance.RegisterListener(
        [this](std::string const & message)
        {
            this->OnLogMessage(message);
        });





    //
    // Initialize OpenGL, creating dummy OpenGL context just for initialization
    //

    int glCanvasAttributes[] =
    {
        WX_GL_RGBA,
        WX_GL_DEPTH_SIZE,      16,
        WX_GL_STENCIL_SIZE,    1,
        0, 0
    };

    mDummyGLCanvas = std::make_unique<wxGLCanvas>(
        this,
        wxID_ANY,
        glCanvasAttributes,
        wxDefaultPosition,
        wxSize(1, 1),
        0L);

    mDummyGLContext = std::make_unique<wxGLContext>(mDummyGLCanvas.get());

    mDummyGLContext->SetCurrent(*mDummyGLCanvas);

    try
    {
        GameOpenGL::InitOpenGL();
    }
    catch (std::exception const & e)
    {
        throw std::runtime_error("Error during OpenGL initialization: " + std::string(e.what()));
    }

    // Register OpenGL context factory
    GPUCalculatorFactory::GetInstance().Initialize(
        []() -> std::unique_ptr<IOpenGLContext>
        {
            return std::make_unique<OpenGLContext>();
        },
        "Shaders/GPUCalc");
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

void MainFrame::OnLogMessage(std::string const & message)
{
    static const std::string TestPassPrefix1 = "TEST_END: PASS";
    static const std::string TestPassPrefix2 = "TEST_RUN_END: PASS";
    static const std::string TestFailPrefix1 = "TEST_FAIL:";
    static const std::string TestFailPrefix2 = "TEST_END: FAIL";
    static const std::string TestFailPrefix3 = "TEST_RUN_END: FAIL";

    if (message.substr(0, TestPassPrefix1.size()) == TestPassPrefix1
        || message.substr(0, TestPassPrefix2.size()) == TestPassPrefix2)
        mLogTextCtrl->SetDefaultStyle(wxTextAttr(wxColor(0, 160, 20)));
    else if (message.substr(0, TestFailPrefix1.size()) == TestFailPrefix1
            || message.substr(0, TestFailPrefix2.size()) == TestFailPrefix2
            || message.substr(0, TestFailPrefix3.size()) == TestFailPrefix3)
        mLogTextCtrl->SetDefaultStyle(wxTextAttr(wxColor(165, 0, 0)));
    else
        mLogTextCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK));

    mLogTextCtrl->AppendText(message);
}

void MainFrame::ClearLog()
{
    mLogTextCtrl->Clear();
}

//////////////////////////////////////////////////////////////////

void MainFrame::RunOpenGLTest()
{
    ClearLog();

    ScopedTestRun testRun;

    OpenGLInitTest test;
    test.Run();
}

void MainFrame::RunPixelCoordsTest(size_t dataPoints)
{
    ClearLog();

    ScopedTestRun testRun;

    PixelCoordsTest test(dataPoints);
    test.Run();
}

void MainFrame::RunAddTest(size_t dataPoints)
{
    ClearLog();

    ScopedTestRun testRun;

    AddTest test(dataPoints);
    test.Run();
}

void MainFrame::RunAllTests()
{
    ClearLog();

    ScopedTestRun testRun;

    {
        OpenGLInitTest test;
        test.Run();
    }

    {
        PixelCoordsTest test(5);
        test.Run();
    }

    {
        PixelCoordsTest test(65536);
        test.Run();
    }

    {
        AddTest test(5);
        test.Run();
    }

    {
        AddTest test(65536);
        test.Run();
    }

    // TODO: all other tests
}