/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewPanel.h"

#include <GameCore/Log.h>

constexpr int TileGap = 5;

ShipPreviewPanel::ShipPreviewPanel(
    wxWindow* parent,
    ResourceLoader const & resourceLoader)
    : wxScrolled<wxPanel>(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE | wxVSCROLL)
    // Preview Thread
    , mPreviewThread()
    , mPanelToThreadMessage()
    , mThreadToPanelMessages()
    , mThreadToPanelMessagesMutex()
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    // TODOTEST
    //Connect(this->GetId(), wxEVT_SHOW, wxShowEventHandler(ShipPreviewPanel::OnShow));
    LogMessage("TODO:ID of panel:", this->GetId());
    Bind(wxEVT_SHOW, &ShipPreviewPanel::OnShow, this, this->GetId());
    Bind(wxEVT_SIZE, &ShipPreviewPanel::OnResized, this, this->GetId());



    //
    // Load bitmaps
    //

    mWaitBitmap = std::make_shared<wxBitmap>(
        resourceLoader.GetBitmapFilepath("ship_preview_wait").string(),
        wxBITMAP_TYPE_PNG);

    mErrorBitmap = std::make_shared<wxBitmap>(
        resourceLoader.GetBitmapFilepath("ship_preview_error").string(),
        wxBITMAP_TYPE_PNG);

    // TODOTEST
    // TODO: check if can register only once, for all IDs
    mPreviewsSizer = new wxGridSizer(2, 2, TileGap, TileGap);
    auto spc1 = new ShipPreviewControl(this, "TODOTEST1", 200, 100, mWaitBitmap, mErrorBitmap);
    spc1->Bind(fsSHIP_FILE_SELECTED, &ShipPreviewPanel::OnShipFileSelected, this);
    mPreviewsSizer->Add(spc1);
    auto spc2 = new ShipPreviewControl(this, "TODOTEST2", 200, 100, mWaitBitmap, mErrorBitmap);
    spc2->Bind(fsSHIP_FILE_SELECTED, &ShipPreviewPanel::OnShipFileSelected, this);
    mPreviewsSizer->Add(spc2);
    auto spc3 = new ShipPreviewControl(this, "TODOTEST3", 200, 100, mWaitBitmap, mErrorBitmap);
    spc3->Bind(fsSHIP_FILE_SELECTED, &ShipPreviewPanel::OnShipFileSelected, this);
    mPreviewsSizer->Add(spc3);
    auto spc4 = new ShipPreviewControl(this, "TODOTEST4", 200, 100, mWaitBitmap, mErrorBitmap);
    spc4->Bind(fsSHIP_FILE_SELECTED, &ShipPreviewPanel::OnShipFileSelected, this);
    mPreviewsSizer->Add(spc4);


    SetSizerAndFit(mPreviewsSizer);

    // TODOTEST
    this->Show();
}

ShipPreviewPanel::~ShipPreviewPanel()
{
}

void ShipPreviewPanel::OnShow(wxShowEvent & event)
{
    // TODO
    if (event.IsShown())
        LogMessage("ShipPreviewPanel::Shown");
    else
        LogMessage("ShipPreviewPanel::Not Shown");
}

void ShipPreviewPanel::OnResized(wxSizeEvent & event)
{
    // TODO
    LogMessage("ShipPreviewPanel::OnResized(", event.GetSize().GetWidth(), ", ", event.GetSize().GetHeight(), ")");
}

void ShipPreviewPanel::OnShipFileSelected(fsShipFileSelectedEvent & event)
{
    // TODO
    LogMessage("ShipPreviewPanel::OnShipFileSelected(", event.GetShipFilepath().string(), ")");
}

void ShipPreviewPanel::SetDirectory(std::filesystem::path const & directoryPath)
{
    // TODOHERE
    LogMessage("ShipPreviewPanel::SetDirectory(", directoryPath.string(), ")");
}

///////////////////////////////////////////////////////////////////////////////////
// Preview Thread
///////////////////////////////////////////////////////////////////////////////////

void ShipPreviewPanel::RunPreviewThread()
{
    // TODO
}

void ShipPreviewPanel::FireThreadToPanelMessageReady()
{
    // TODO: post custom event - see https://wiki.wxwidgets.org/Custom_Events and https://stackoverflow.com/questions/4028507/using-templated-custom-wxevent-with-wxevthandlerbind
    // TODO: make OnThreadToPanelMessageReady an event handler, and bind above
}

void ShipPreviewPanel::OnThreadToPanelMessageReady()
{
    //
    // Process queue
    //

    while (true)
    {
        //
        // Pop message
        //

        std::unique_ptr<IThreadToPanelMessage> message;

        {
            std::lock_guard<std::mutex> lock(mThreadToPanelMessagesMutex);
            if (!mThreadToPanelMessages.empty())
            {
                message = std::move(mThreadToPanelMessages.front());
                mThreadToPanelMessages.pop_front();
            }
        }

        if (!message)
            break; // We're done

        //
        // Process message
        //

        // TODO: switch
    }
}