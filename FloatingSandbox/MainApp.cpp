/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

//
// The main application. This journey begins from here.
//

#include "MainFrame.h"
#include "UnhandledExceptionHandler.h"

#include <GameCore/FloatingPoint.h>

#include <wx/app.h>
#include <wx/msgdlg.h>

#ifdef _MSC_VER
// Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

#ifdef _DEBUG
#ifdef _MSC_VER
#include <crtdbg.h>
#include <signal.h>
#include <stdlib.h>
#include <WinBase.h>

void SignalHandler(int signal)
{
    if (signal == SIGABRT)
    {
        // Break the VS debugger here
        RaiseException(0x40010005, 0, 0, 0);
    }
}

#endif
#endif

class MainApp : public wxApp
{
public:

    virtual bool OnInit() override;

    virtual int FilterEvent(wxEvent & event) override;

private:

    MainFrame * mMainFrame{ nullptr };
};

IMPLEMENT_APP(MainApp);

bool MainApp::OnInit()
{
    //
    // Install handler for unhandled exceptions
    //

    InstallUnhandledExceptionHandler();


    //
    // Initialize assert handling
    //

#ifdef _DEBUG
#ifdef _MSC_VER

    //
    // We configure assertion failures to not show the assert window but rather
    // to write to stderr; they will then invoke abort(), which we hook to raise
    // a win32 exception so that the MSVC debugger breaks on it.
    //

    _set_error_mode(_OUT_TO_STDERR);

    _set_abort_behavior(0, _WRITE_ABORT_MSG);
    signal(SIGABRT, SignalHandler);

#endif
#endif

    //
    // Initialize floating point handling
    //

    // Avoid denormal numbers for very small quantities
    EnableFloatingPointFlushToZero();

#ifdef FLOATING_POINT_CHECKS
    EnableFloatingPointExceptions();
#endif

    //
    // Initialize wxWidgets
    //

    wxInitAllImageHandlers();



    //
    // Create frame and start
    //

    try
    {
        mMainFrame = new MainFrame(
            this,
            wxICON(BBB_SHIP_ICON));

        SetTopWindow(mMainFrame);

        return true;
    }
    catch (std::exception const & e)
    {
        wxMessageBox(std::string(e.what()), wxT("Error"), wxICON_ERROR);

        return false;
    }
}

int MainApp::FilterEvent(wxEvent & event)
{
    // This is the only way for us to catch KEY_UP events
    if (event.GetEventType() == wxEVT_KEY_UP
        && nullptr != mMainFrame)
    {
        bool isProcessed = mMainFrame->ProcessKeyUp(
            ((wxKeyEvent&)event).GetKeyCode(),
            ((wxKeyEvent&)event).GetModifiers());

        if (isProcessed)
            return Event_Processed;
    }

    // Not handled, continue processing
    return Event_Skip;
}