/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <UILib/LocalizationManager.h>

#include <Game/ResourceLocator.h>

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/timer.h>

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
        LocalizationManager & localizationManager);

private:

    wxApp * const mMainApp;

    //
    // Helpers
    //

    ResourceLocator const & mResourceLocator;
    LocalizationManager & mLocalizationManager;
};

}