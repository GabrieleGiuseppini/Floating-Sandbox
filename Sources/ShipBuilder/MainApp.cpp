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

#include <Game/GameAssetManager.h>
#include <Game/GameVersion.h>

#include <Simulation/MaterialDatabase.h>
#include <Simulation/ShipTexturizer.h>

#include <Core/BuildInfo.h>
#include <Core/Log.h>

#include <wx/app.h>
#include <wx/msgdlg.h>

#include <memory>

#ifdef _MSC_VER
// Nothing to do, we use resources
#else
#include "Resources/ShipBBB.xpm"
#endif

#ifdef _DEBUG
#ifdef _MSC_VER
#include <signal.h>
#include <stdlib.h>

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

private:

    ShipBuilder::MainFrame * mMainFrame;
    std::unique_ptr<GameAssetManager> mGameAssetManager;
    std::unique_ptr<LocalizationManager> mLocalizationManager;
    std::unique_ptr<MaterialDatabase> mMaterialDatabase;
    std::unique_ptr<ShipTexturizer> mShipTexturizer;
};

IMPLEMENT_APP(MainApp);

MainApp::MainApp()
    : mMainFrame(nullptr)
{
    //
    // Bootstrap log
    //

    // Log full app name, current build info, and today's date
    LogMessage(std::string(APPLICATION_NAME_WITH_LONG_VERSION), " ", BuildInfo::GetBuildInfo().ToString(), " @ ", Utils::MakeTodayDateString());
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

        mGameAssetManager = std::make_unique<GameAssetManager>(std::string(argv[0]));

        //
        // Initialize wxWidgets and language used for localization
        //

        // Image handlers
        wxInitAllImageHandlers();

        // Language (system default)
        mLocalizationManager = LocalizationManager::CreateInstance(std::nullopt, *mGameAssetManager);

        //
        // Initialize helpers
        //

        mMaterialDatabase = std::make_unique<MaterialDatabase>(std::move(MaterialDatabase::Load(*mGameAssetManager)));
        mShipTexturizer = std::make_unique<ShipTexturizer>(
            *mMaterialDatabase,
            *mGameAssetManager);

        //
        // Create frame
        //

        mMainFrame = new ShipBuilder::MainFrame(
            this,
            wxICON(BBB_SHIP_ICON),
            *mGameAssetManager,
            *mLocalizationManager,
            *mMaterialDatabase,
            *mShipTexturizer,
            nullptr,
            {},
            ProgressCallback([](float, ProgressMessageType){}));

        SetTopWindow(mMainFrame);

        //
        // Run
        //

        mMainFrame->OpenForNewShip(std::optional<UnitsSystem>());

        return true;

    }
    catch (std::exception const & e)
    {
        wxMessageBox(std::string(e.what()), wxT("Error"), wxICON_ERROR);

        // Abort
        return false;
    }
}