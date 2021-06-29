/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

//
// The main application. This journey begins from here.
//

#include "LocalizationManager.h"
#include "MainFrame.h"
#include "UIPreferencesManager.h"
#include "UnhandledExceptionHandler.h"

#include <Game/ResourceLocator.h>

#include <GameCore/FloatingPoint.h>
#include <GameCore/SysSpecifics.h>

#include <wx/app.h>
#include <wx/msgdlg.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

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

    MainApp();

    virtual bool OnInit() override;

    virtual int FilterEvent(wxEvent & event) override;

private:

    bool RunSecretTypingStateMachine(wxKeyEvent const & keyEvent);

private:

    MainFrame * mMainFrame;
    std::unique_ptr<ResourceLocator> mResourceLocator;
    std::unique_ptr<LocalizationManager> mLocalizationManager;


    //
    // Secret typing state machine
    //

    std::optional<std::string> mCurrentSecretTypingSequence{ std::nullopt };
    std::vector<std::pair<std::string, std::function<void()>>> mSecretTypingMappings;
};

IMPLEMENT_APP(MainApp);

#if FS_IS_OS_LINUX()
#include <X11/Xlib.h>
#endif

MainApp::MainApp()
    : mMainFrame(nullptr)
    , mLocalizationManager()
{
#if FS_IS_OS_LINUX()
    //
    // Initialize multi-threading in X-Windows
    //

    XInitThreads();
#endif
}

bool MainApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;


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
    // Dump system timer resolution
    //

#ifdef _MSC_VER
    HMODULE const hNtDll = ::GetModuleHandle(L"Ntdll");
    if (hNtDll != NULL)
    {
        ULONG nMinRes, nMaxRes, nCurRes;
        typedef NTSTATUS(CALLBACK * LPFN_NtQueryTimerResolution)(PULONG, PULONG, PULONG);
        auto const pQueryResolution = (LPFN_NtQueryTimerResolution)::GetProcAddress(hNtDll, "NtQueryTimerResolution");
        if (pQueryResolution != nullptr &&
            pQueryResolution(&nMinRes, &nMaxRes, &nCurRes) == ERROR_SUCCESS)
        {
            LogMessage("Windows timer resolution (min/max/cur): ",
                nMinRes / 10000, ".", (nMinRes % 10000) / 10, " / ",
                nMaxRes / 10000, ".", (nMaxRes % 10000) / 10, " / ",
                nCurRes / 10000, ".", (nCurRes % 10000) / 10, " ms");
        }
    }
#endif

    try
    {
        //
        // Initialize resource locator
        //

        mResourceLocator = std::make_unique<ResourceLocator>(std::string(argv[0]));

        //
        // Initialize wxWidgets and language used for localization
        //

        // Image handlers
        wxInitAllImageHandlers();

        // Language
        auto const preferredLanguage = UIPreferencesManager::LoadPreferredLanguage();
        mLocalizationManager = LocalizationManager::CreateInstance(preferredLanguage, *mResourceLocator);

        //
        // Create frame
        //

        mMainFrame = new MainFrame(this, *mResourceLocator , *mLocalizationManager);

        SetTopWindow(mMainFrame);

        //
        // Initialize secret typing mappings
        //

        mSecretTypingMappings.emplace_back("BOOTSETTINGS", std::bind(&MainFrame::OnSecretTypingBootSettings, mMainFrame));
        mSecretTypingMappings.emplace_back("DEBUG", std::bind(&MainFrame::OnSecretTypingDebug, mMainFrame));
        mSecretTypingMappings.emplace_back("BUILTINSHIP1", std::bind(&MainFrame::OnSecretTypingLoadBuiltInShip, mMainFrame, 1));
        mSecretTypingMappings.emplace_back("BUILTINSHIP2", std::bind(&MainFrame::OnSecretTypingLoadBuiltInShip, mMainFrame, 2));
        mSecretTypingMappings.emplace_back("BUILTINSHIP3", std::bind(&MainFrame::OnSecretTypingLoadBuiltInShip, mMainFrame, 3));
        mSecretTypingMappings.emplace_back("LEFT", std::bind(&MainFrame::OnSecretTypingGoToWorldEnd, mMainFrame, 0));
        mSecretTypingMappings.emplace_back("RIGHT", std::bind(&MainFrame::OnSecretTypingGoToWorldEnd, mMainFrame, 1));

        //
        // Run
        //

        return true;

    }
    catch (std::exception const & e)
    {
        wxMessageBox(std::string(e.what()), wxT("Error"), wxICON_ERROR);

        // Abort
        return false;
    }
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

            bool const isProcessed = mMainFrame->ProcessKeyUp(
                keyEvent.GetKeyCode(),
                keyEvent.GetModifiers());

            if (isProcessed)
                return Event_Processed;
        }
        else if (event.GetEventType() == wxEVT_KEY_DOWN
            || event.GetEventType() == wxEVT_CHAR
            || event.GetEventType() == wxEVT_CHAR_HOOK)
        {
            wxKeyEvent const & keyEvent = static_cast<wxKeyEvent const &>(event);

            //
            // Run secret typing state machine and, if not processed,
            // allow event to continue its path
            //

            if (RunSecretTypingStateMachine(keyEvent))
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