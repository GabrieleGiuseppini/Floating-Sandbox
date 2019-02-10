/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewPanel.h"

#include <Game/ImageFileTools.h>
#include <Game/ShipDefinitionFile.h>

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

wxDEFINE_EVENT(fsEVT_DIR_SCANNED, fsDirScannedEvent);
wxDEFINE_EVENT(fsEVT_DIR_SCAN_ERROR, fsDirScanErrorEvent);
wxDEFINE_EVENT(fsEVT_PREVIEW_READY, fsPreviewReadyEvent);
wxDEFINE_EVENT(fsEVT_PREVIEW_ERROR, fsPreviewErrorEvent);
wxDEFINE_EVENT(fsEVT_DIR_PREVIEW_COMPLETE, fsDirPreviewCompleteEvent);

ShipPreviewPanel::ShipPreviewPanel(
    wxWindow* parent,
    ResourceLoader const & resourceLoader)
    : wxScrolled<wxPanel>(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE | wxVSCROLL)
    , mWidth(0)
    , mHeight(0)
    , mPreviewPanel(nullptr)
    , mPreviewPanelSizer(nullptr)
    , mPreviewControls()
    , mSelectedPreview(std::nullopt)
    , mWaitImage(ImageFileTools::LoadImageRgbaLowerLeft(resourceLoader.GetBitmapFilepath("ship_preview_wait")))
    , mErrorImage(ImageFileTools::LoadImageRgbaLowerLeft(resourceLoader.GetBitmapFilepath("ship_preview_error")))
    , mCurrentlyCompletedDirectory()
    // Preview Thread
    , mPreviewThread()
    , mPanelToThreadMessage()
    , mPanelToThreadMessageMutex()
    , mPanelToThreadMessageLock(mPanelToThreadMessageMutex, std::defer_lock)
    , mPanelToThreadMessageEvent()
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetScrollRate(5, 5);

    // Ensure one tile always fits, accounting for the V scrollbar
    SetMinSize(wxSize(MinPreviewWidth + 20, -1));

    Bind(wxEVT_SIZE, &ShipPreviewPanel::OnResized, this, this->GetId());

    // Register for the thread events
    Bind(fsEVT_DIR_SCANNED, &ShipPreviewPanel::OnDirScanned, this);
    Bind(fsEVT_DIR_SCAN_ERROR, &ShipPreviewPanel::OnDirScanError, this);
    Bind(fsEVT_PREVIEW_READY, &ShipPreviewPanel::OnPreviewReady, this);
    Bind(fsEVT_PREVIEW_ERROR, &ShipPreviewPanel::OnPreviewError, this);
    Bind(fsEVT_DIR_PREVIEW_COMPLETE, &ShipPreviewPanel::OnDirPreviewComplete, this);

    // Make our own sizer
    wxBoxSizer * panelSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(panelSizer);
}

ShipPreviewPanel::~ShipPreviewPanel()
{
    // Stop thread
    if (mPreviewThread.joinable())
    {
        ShutdownPreviewThread();
    }
}

void ShipPreviewPanel::OnOpen()
{
    assert(!mSelectedPreview);

    //
    // Start thread
    //

    assert(!mPreviewThread.joinable());
    mPreviewThread = std::thread(&ShipPreviewPanel::RunPreviewThread, this);
}

void ShipPreviewPanel::OnClose()
{
    //
    // Stop thread
    //

    assert(mPreviewThread.joinable());
    ShutdownPreviewThread();


    //
    // Clear state
    //

    if (!!mSelectedPreview)
    {
        assert(*mSelectedPreview < mPreviewControls.size());
        mPreviewControls[*mSelectedPreview]->SetSelected(false);
    }

    mSelectedPreview.reset();
}

void ShipPreviewPanel::SetDirectory(std::filesystem::path const & directoryPath)
{
    // Check if different than current
    if (directoryPath != mCurrentlyCompletedDirectory)
    {
        //
        // Change directory
        //

        mCurrentlyCompletedDirectory.reset();

        // Clear state
        mSelectedPreview.reset();

        // Tell thread (if it's running)
        mPanelToThreadMessageLock.lock();
        mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeSetDirectoryMessage(directoryPath)));
        mPanelToThreadMessageEvent.notify_one();
        mPanelToThreadMessageLock.unlock();
    }
}

void ShipPreviewPanel::OnResized(wxSizeEvent & event)
{
    LogMessage("ShipPreviewPanel::OnResized(", event.GetSize().GetWidth(), ", ", event.GetSize().GetHeight(), ")");

    // Store size
    mWidth = event.GetSize().GetWidth();
    mHeight = event.GetSize().GetHeight();

    // Rearrange
    if (nullptr != mPreviewPanelSizer)
    {
        //
        // Rearrange tiles based on width
        //

        int nCols = CalculateTileColumns();
        if (nCols != mPreviewPanelSizer->GetCols())
        {
            //
            // Rearrange
            //

            Freeze();

            mPreviewPanelSizer->SetCols(nCols);
            mPreviewPanel->Layout();
            mPreviewPanelSizer->SetSizeHints(mPreviewPanel);
            this->Refresh();

            Thaw();
        }
    }

    event.Skip();
}

void ShipPreviewPanel::OnDirScanned(fsDirScannedEvent & event)
{
    //
    // Create new panel
    //

    wxPanel * newPreviewPanel = new wxPanel();
    newPreviewPanel->Hide();
    newPreviewPanel->Create(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxGridSizer * newPreviewPanelSizer = nullptr;

    std::vector<ShipPreviewControl *> newPreviewControls;

    if (!event.GetShipFilepaths().empty())
    {

        //
        // Create new preview controls
        //

        newPreviewPanelSizer = new wxGridSizer(CalculateTileColumns(), 0, 0);

        for (size_t shipIndex = 0; shipIndex < event.GetShipFilepaths().size(); ++shipIndex)
        {
            auto shipPreviewControl = new ShipPreviewControl(
                newPreviewPanel,
                shipIndex,
                event.GetShipFilepaths()[shipIndex],
                PreviewVGap,
                mWaitImage,
                mErrorImage);

            newPreviewControls.push_back(shipPreviewControl);

            // Register for preview selections
            shipPreviewControl->Bind(fsEVT_SHIP_FILE_SELECTED, &ShipPreviewPanel::OnShipFileSelected, this);

            // Add to sizer
            newPreviewPanelSizer->Add(shipPreviewControl, 0, wxALIGN_CENTRE_HORIZONTAL | wxALIGN_TOP);
        }

        newPreviewPanel->SetSizerAndFit(newPreviewPanelSizer);
    }
    else
    {
        //
        // Just tell the user there are no ships here
        //

        auto labelSizer = new wxBoxSizer(wxVERTICAL);

        auto label = new wxStaticText(
            newPreviewPanel,
            wxID_ANY,
            "There are no ships in this folder",
            wxDefaultPosition,
            wxDefaultSize,
            wxALIGN_CENTER_HORIZONTAL);

        labelSizer->AddStretchSpacer(1);
        labelSizer->Add(label, 0, wxEXPAND);
        labelSizer->AddStretchSpacer(1);

        newPreviewPanel->SetSizerAndFit(labelSizer);
    }


    //
    // Swap panels
    //

    if (mPreviewPanel != nullptr)
        mPreviewPanel->Destroy(); // Will also destroy sizer and preview controls

    mPreviewControls = newPreviewControls;

    mPreviewPanel = newPreviewPanel;
    mPreviewPanelSizer = newPreviewPanelSizer;


    //
    // Add panel to our sizer
    //

    assert(nullptr != GetSizer());
    GetSizer()->Clear();
    GetSizer()->Add(mPreviewPanel, 1, wxEXPAND);
    GetSizer()->Layout();

    mPreviewPanel->Show();

    // Re-trigger scroll bar
    this->FitInside();
}

void ShipPreviewPanel::OnDirScanError(fsDirScanErrorEvent & event)
{
    throw GameException(event.GetErrorMessage());
}

void ShipPreviewPanel::OnPreviewReady(fsPreviewReadyEvent & event)
{
    assert(event.GetShipIndex() < mPreviewControls.size());
    mPreviewControls[event.GetShipIndex()]->SetPreviewContent(*(event.GetShipPreview()));
}

void ShipPreviewPanel::OnPreviewError(fsPreviewErrorEvent & event)
{
    assert(event.GetShipIndex() < mPreviewControls.size());
    mPreviewControls[event.GetShipIndex()]->SetPreviewContent(mErrorImage, event.GetErrorMessage(), "");
}

void ShipPreviewPanel::OnDirPreviewComplete(fsDirPreviewCompleteEvent & event)
{
    // Remember the current directory, now that it's complete
    mCurrentlyCompletedDirectory = event.GetDirectoryPath();
}

void ShipPreviewPanel::OnShipFileSelected(fsShipFileSelectedEvent & event)
{
    //
    // Toggle selection
    //

    if (!!mSelectedPreview)
    {
        assert(*mSelectedPreview < mPreviewControls.size());
        mPreviewControls[*mSelectedPreview]->SetSelected(false);
    }

    assert(event.GetShipIndex() < mPreviewControls.size());
    mPreviewControls[event.GetShipIndex()]->SetSelected(true);

    mSelectedPreview = event.GetShipIndex();

    // Propagate up
    ProcessWindowEvent(event);
}

int ShipPreviewPanel::CalculateTileColumns()
{
    int nCols = static_cast<int>(static_cast<float>(mWidth) / static_cast<float>(MinPreviewWidth));
    assert(nCols >= 1);

    return nCols;
}

void ShipPreviewPanel::ShutdownPreviewThread()
{
    mPanelToThreadMessageLock.lock();
    mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeExitMessage()));
    mPanelToThreadMessageEvent.notify_one();
    mPanelToThreadMessageLock.unlock();

    // Wait for thread to be done
    mPreviewThread.join();
}

///////////////////////////////////////////////////////////////////////////////////
// Preview Thread
///////////////////////////////////////////////////////////////////////////////////

void ShipPreviewPanel::RunPreviewThread()
{
    LogMessage("PreviewThread::Enter");

    std::unique_lock<std::mutex> messageThreadLock(mPanelToThreadMessageMutex, std::defer_lock);

    while (true)
    {
        //
        // Check whether there's a message for us
        //
        // Note that we will always see the latest message
        //

        std::unique_ptr<PanelToThreadMessage> message;

        // Lock now so that we can guarantee that the condition variable
        // won't be set before we're waiting for it
        messageThreadLock.lock();
        if (!mPanelToThreadMessage)
        {
            // No message, wait
            // Mutex is currently locked, and it will be unlocked while we are waiting
            mPanelToThreadMessageEvent.wait(messageThreadLock);
            // Mutex is now locked again
        }

        // Got a message, extract it (we're holding the lock)
        assert(!!mPanelToThreadMessage);
        message = std::move(mPanelToThreadMessage);

        // Unlock while we process
        messageThreadLock.unlock();

        //
        // Process Message
        //

        if (PanelToThreadMessage::MessageType::Exit == message->GetMessageType())
        {
            //
            // Exit
            //

            break;
        }
        else
        {
            assert(PanelToThreadMessage::MessageType::SetDirectory == message->GetMessageType());

            //
            // Set directory
            //

            try
            {
                ScanDirectory(message->GetDirectoryPath());
            }
            catch (std::exception const & ex)
            {
                // Fire error event
                QueueEvent(
                    new fsDirScanErrorEvent(
                        fsEVT_DIR_SCAN_ERROR,
                        this->GetId(),
                        ex.what()));
            }
        }
    }

    LogMessage("PreviewThread::Exit");
}

void ShipPreviewPanel::ScanDirectory(std::filesystem::path const & directoryPath)
{
    LogMessage("PreviewThread::ScanDirectory(", directoryPath.string(), ")");

    bool isSingleCore = (std::thread::hardware_concurrency() < 2);


    //
    // Get listings and fire event
    //

    std::vector<std::filesystem::path> shipFilepaths;

    try
    {
        for (auto const & entryIt : std::filesystem::directory_iterator(directoryPath))
        {
            auto const entryFilepath = entryIt.path();
            try
            {
                if (std::filesystem::is_regular_file(entryFilepath)
                    && (entryFilepath.extension().string() == ".png" || ShipDefinitionFile::IsShipDefinitionFile(entryFilepath)))
                {
                    shipFilepaths.push_back(entryFilepath);
                }
            }
            catch (...)
            { /*ignore this file*/ }
        }
    }
    catch (...)
    { /* interrupt scan here */ }

    QueueEvent(
        new fsDirScannedEvent(
            fsEVT_DIR_SCANNED,
            this->GetId(),
            shipFilepaths));

    if (isSingleCore)
    {
        // Give the main thread time to process this
        std::this_thread::yield();
    }


    //
    // Process all files and create previews
    //

    for (size_t iShip = 0; iShip < shipFilepaths.size(); ++iShip)
    {
        // Check whether we have been interrupted
        if (!!mPanelToThreadMessage)
            return;

        try
        {
            // Load preview
            auto shipPreview = ShipPreview::Load(
                shipFilepaths[iShip],
                ImageSize(ShipPreviewControl::ImageWidth, ShipPreviewControl::ImageHeight));

            // Fire event
            QueueEvent(
                new fsPreviewReadyEvent(
                    fsEVT_PREVIEW_READY,
                    this->GetId(),
                    iShip,
                    std::make_shared<ShipPreview>(std::move(shipPreview))));

            if (isSingleCore)
            {
                // Give the main thread time to process this
                std::this_thread::yield();
            }
        }
        catch (std::exception const & ex)
        {
            // Fire error event
            QueueEvent(
                new fsPreviewErrorEvent(
                    fsEVT_PREVIEW_ERROR,
                    this->GetId(),
                    iShip,
                    ex.what()));
        }
    }


    //
    // Fire completion event
    //

    QueueEvent(
        new fsDirPreviewCompleteEvent(
            fsEVT_DIR_PREVIEW_COMPLETE,
            this->GetId(),
            directoryPath));
}