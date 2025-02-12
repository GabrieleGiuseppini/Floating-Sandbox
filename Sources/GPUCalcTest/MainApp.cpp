/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-05
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include <Core/FloatingPoint.h>

#include "MainFrame.h"

#include <wx/app.h>
#include <wx/msgdlg.h>

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
};

IMPLEMENT_APP(MainApp);

bool MainApp::OnInit()
{
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
    // Create frame and start
    //

    try
    {
        MainFrame* frame = new MainFrame(this);
        frame->Show();
        SetTopWindow(frame);

        return true;
    }
    catch (std::exception const & e)
    {
        wxMessageBox(std::string(e.what()), wxT("Error"), wxICON_ERROR);

        return false;
    }
}