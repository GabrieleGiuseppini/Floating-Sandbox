/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewPanel.h"

#include <Game/ShipDefinitionFile.h>

#include <GameCore/Log.h>

constexpr int PreviewWidth = 200;
constexpr int PreviewHeight = 100;
constexpr int PreviewMargin = 5;

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
    , mCurrentDirectory()
    // Preview Thread
    , mPreviewThread()
    , mPanelToThreadMessage()
    , mPanelToThreadMessageMutex()
    , mPanelToThreadMessageLock(mPanelToThreadMessageMutex, std::defer_lock)
    , mPanelToThreadMessageEvent()
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetScrollRate(5, 5);

    // Ensure one tile always fits
    SetMinSize(wxSize(PreviewWidth + 2 * PreviewMargin, PreviewHeight));

    Bind(wxEVT_SIZE, &ShipPreviewPanel::OnResized, this, this->GetId());

    // Register for the thread events
    Bind(fsEVT_DIR_SCANNED, &ShipPreviewPanel::OnDirScanned, this);
    Bind(fsEVT_DIR_SCAN_ERROR, &ShipPreviewPanel::OnDirScanError, this);
    Bind(fsEVT_PREVIEW_READY, &ShipPreviewPanel::OnPreviewReady, this);
    Bind(fsEVT_PREVIEW_ERROR, &ShipPreviewPanel::OnPreviewError, this);
    Bind(fsEVT_DIR_PREVIEW_COMPLETE, &ShipPreviewPanel::OnDirPreviewComplete, this);


    //
    // Load bitmaps
    //

    mWaitBitmap = std::make_shared<wxBitmap>(
        resourceLoader.GetBitmapFilepath("ship_preview_wait").string(),
        wxBITMAP_TYPE_PNG);

    mErrorBitmap = std::make_shared<wxBitmap>(
        resourceLoader.GetBitmapFilepath("ship_preview_error").string(),
        wxBITMAP_TYPE_PNG);


    //
    // Create sizer
    //

    mPreviewsSizer = new wxGridSizer(1, 0, 0);

    SetSizer(mPreviewsSizer);
}

ShipPreviewPanel::~ShipPreviewPanel()
{
}

void ShipPreviewPanel::OnOpen()
{
    // Start thread
    assert(!mPreviewThread.joinable());
    mPreviewThread = std::thread(&ShipPreviewPanel::RunPreviewThread, this);
}

void ShipPreviewPanel::OnClose()
{
    // Stop thread
    assert(mPreviewThread.joinable());
    mPanelToThreadMessageLock.lock();
    mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeExitMessage()));
    mPanelToThreadMessageEvent.notify_one();
    mPanelToThreadMessageLock.unlock();

    // Wait for thread to be done
    mPreviewThread.join();
}

void ShipPreviewPanel::SetDirectory(std::filesystem::path const & directoryPath)
{
    // Check if different than current
    if (directoryPath != mCurrentDirectory)
    {
        // Change directory
        mCurrentDirectory = directoryPath;

        // Tell thread (if it's running)
        mPanelToThreadMessageLock.lock();
        mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeSetDirectoryMessage(mCurrentDirectory)));
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

    // See if need to rearrange
    if (!mPreviewControls.empty())
    {
        ArrangePreviewTiles();
    }
}

void ShipPreviewPanel::ArrangePreviewTiles()
{
    //
    // Rearrange tiles based on width
    //

    int nCols = static_cast<int>(static_cast<float>(mWidth) / static_cast<float>(PreviewWidth + 2 * PreviewMargin));
    assert(nCols >= 1);

    // Rearrange
    mPreviewsSizer->SetCols(nCols);
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
            // Exit
            break;
        }
        else
        {
            // Set directory
            assert(PanelToThreadMessage::MessageType::SetDirectory == message->GetMessageType());

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


    //
    // Get listings and fire event
    //

    std::vector<std::filesystem::path> shipFilepaths;

    for (auto const & entryIt : std::filesystem::directory_iterator(directoryPath))
    {
        auto const entryFilepath = entryIt.path();
        if (std::filesystem::is_regular_file(entryFilepath)
            && (entryFilepath.extension().string() == ".png" || ShipDefinitionFile::IsShipDefinitionFile(entryFilepath)))
        {
            shipFilepaths.push_back(entryFilepath);
        }
    }

    QueueEvent(
        new fsDirScannedEvent(
            fsEVT_DIR_SCANNED,
            this->GetId(),
            shipFilepaths));


    //
    // Process all files and create previews
    //

    for (auto const & shipFilepath : shipFilepaths)
    {
        // Check whether we have been interrupted
        if (!!mPanelToThreadMessage)
            return;

        // TODO
    }


    //
    // Fire completion event
    //

    QueueEvent(
        new fsDirPreviewCompleteEvent(
            fsEVT_DIR_PREVIEW_COMPLETE,
            this->GetId()));
}

void ShipPreviewPanel::OnDirScanned(fsDirScannedEvent & event)
{
    //
    // Clear all preview controls
    //

    mPreviewsSizer->Clear(true);
    mPreviewControls.clear();


    //
    // Create new preview controls
    //

    for (auto const & shipFilepath : event.GetShipFilepaths())
    {
        auto shipPreviewControl = new ShipPreviewControl(
            this,
            shipFilepath,
            PreviewWidth,
            PreviewHeight,
            PreviewMargin,
            mWaitBitmap,
            mErrorBitmap);

        mPreviewControls.push_back(shipPreviewControl);

        // Add to sizer
        mPreviewsSizer->Add(shipPreviewControl, 0, wxALIGN_CENTRE_HORIZONTAL | wxALIGN_TOP);
    }


    //
    // Arrange controls
    //

    ArrangePreviewTiles();

    mPreviewsSizer->Layout();
    FitInside();
}

void ShipPreviewPanel::OnDirScanError(fsDirScanErrorEvent & event)
{
    // TODO
}

void ShipPreviewPanel::OnPreviewReady(fsPreviewReadyEvent & event)
{
    // TODO
}

void ShipPreviewPanel::OnPreviewError(fsPreviewErrorEvent & event)
{
    // TODO
}

void ShipPreviewPanel::OnDirPreviewComplete(fsDirPreviewCompleteEvent & event)
{
    // TODO
}