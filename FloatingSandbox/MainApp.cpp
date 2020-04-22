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

#include <functional>
#include <optional>
#include <string>

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

    bool RunSecretTypingStateMachine(wxKeyEvent const & keyEvent);

private:

    MainFrame * mMainFrame{ nullptr };


    //
    // Secret typing state machine
    //

    std::optional<std::string> mCurrentSecretTypingSequence{ std::nullopt };
    std::vector<std::pair<std::string, std::function<void()>>> mSecretTypingMappings;
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
    // Create frame
    //

    try
    {
        mMainFrame = new MainFrame(
            this,
            wxICON(BBB_SHIP_ICON));

        SetTopWindow(mMainFrame);
    }
    catch (std::exception const & e)
    {
        wxMessageBox(std::string(e.what()), wxT("Error"), wxICON_ERROR);

        // Abort
        return false;
    }


    //
    // Initialize secret typing mappings
    //

    mSecretTypingMappings.emplace_back("DEBUG", std::bind(&MainFrame::OnSecretTypingOpenDebugWindow, mMainFrame));
    mSecretTypingMappings.emplace_back("FALLBACK", std::bind(&MainFrame::OnSecretTypingLoadFallbackShip, mMainFrame));


    //
    // Run
    //

    return true;
}

int MainApp::FilterEvent(wxEvent & event)
{
    if (nullptr != mMainFrame)
    {
        // This is the only way for us to catch KEY_UP events
        if (event.GetEventType() == wxEVT_KEY_UP)
        {
            //
            // Forward to main frame
            //

            wxKeyEvent const & keyEvent = static_cast<wxKeyEvent const &>(event);

            bool isProcessed = mMainFrame->ProcessKeyUp(
                keyEvent.GetKeyCode(),
                keyEvent.GetModifiers());

            if (isProcessed)
                return Event_Processed;
        }
        else if (event.GetEventType() == wxEVT_KEY_DOWN
            || event.GetEventType() == wxEVT_CHAR
            || event.GetEventType() == wxEVT_CHAR_HOOK)
        {
            //
            // Run secret typing state machine and, if not processed,
            // forward to main frame
            //

            wxKeyEvent const & keyEvent = static_cast<wxKeyEvent const &>(event);

            if (RunSecretTypingStateMachine(keyEvent))
            {
                return Event_Processed;
            }
            else if (mMainFrame->ProcessKeyDown(keyEvent.GetKeyCode(), keyEvent.GetModifiers()))
            {
                return Event_Processed;
            }
        }
    }

    // Event not handled, continue processing
    return Event_Skip;
}

bool MainApp::RunSecretTypingStateMachine(wxKeyEvent const & keyEvent)
{
    if (keyEvent.GetKeyCode() == 'D' && keyEvent.GetModifiers() == wxMOD_ALT)
    {
        // Start/restart state machine
        mCurrentSecretTypingSequence.emplace("");

        // Event handled
        return true;
    }
    else if (mCurrentSecretTypingSequence.has_value())
    {
        if (keyEvent.GetModifiers() == wxMOD_NONE
            && keyEvent.GetKeyCode() >= 0x20
            && keyEvent.GetKeyCode() <= 0x7f)
        {
            // Add character
            mCurrentSecretTypingSequence->push_back(static_cast<char>(keyEvent.GetKeyCode()));

            // Check whether it's a (partial or full) match against our mappings
            bool atLeastOneMatch = false;
            for (auto const & mapping : mSecretTypingMappings)
            {
                if (0 == mapping.first.compare(0, mCurrentSecretTypingSequence->length(), *mCurrentSecretTypingSequence))
                {
                    // Match (partial or full)
                    atLeastOneMatch = true;

                    // Check whether full match
                    if (mapping.first.length() == mCurrentSecretTypingSequence->length())
                    {
                        // Full match!

                        // Reset state machine
                        // ...interrupt state machine
                        mCurrentSecretTypingSequence.reset();

                        // Handle event
                        mapping.second();

                        // Event handled
                        return true;
                    }
                }
            }

            if (!atLeastOneMatch)
            {
                // Completely wrong key...
                // ...interrupt state machine
                mCurrentSecretTypingSequence.reset();
            }

            // Event handled
            return true;
        }
        else
        {
            // Interrupt state machine
            mCurrentSecretTypingSequence.reset();

            // Event handled
            return true;
        }
    }

    // Event not handled, continue processing
    return false;
}