/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

//
// The main application. This journey begins from here.
//

#include <GameLib/FloatingPoint.h>

#include "MainFrame.h"

#include <wx/app.h>

class MainApp : public wxApp
{
public:
    virtual bool OnInit() override;
};

IMPLEMENT_APP(MainApp);

bool MainApp::OnInit()
{
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

    MainFrame* frame = new MainFrame(this);
    frame->SetIcon(wxICON(AAA_SHIP_ICON));
    SetTopWindow(frame);
    
    return true;
}