/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

//
// The main ShipBuilder application.
//

#include <ShipBuilderLib/MainFrame.h>

#include <UILib/LocalizationManager.h>

#include <Game/ResourceLocator.h>

#include <wx/app.h>
#include <wx/msgdlg.h>

#include <optional>
#include <string>

class MainApp : public wxApp
{
public:

    MainApp();

    virtual bool OnInit() override;

private:

    ShipBuilder::MainFrame * mMainFrame;
    std::unique_ptr<ResourceLocator> mResourceLocator;
    std::unique_ptr<LocalizationManager> mLocalizationManager;
};

IMPLEMENT_APP(MainApp);

MainApp::MainApp()
    : mMainFrame(nullptr)
    , mLocalizationManager()
{
}

bool MainApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

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
        std::optional<std::string> preferredLanguage = std::nullopt; // TODO: load language from Preferences here
        mLocalizationManager = LocalizationManager::CreateInstance(preferredLanguage, *mResourceLocator);

        //
        // Create frame
        //

        mMainFrame = new ShipBuilder::MainFrame(this, *mResourceLocator , *mLocalizationManager);

        SetTopWindow(mMainFrame);

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