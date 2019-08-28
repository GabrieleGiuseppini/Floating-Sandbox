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
    , mShipNameToPreviewIndex()
    // Preview Thread
    , mPreviewThread()
    , mPanelToThreadMessage()
    , mPanelToThreadMessageMutex()
    , mPanelToThreadMessageLock(mPanelToThreadMessageMutex, std::defer_lock)
    , mPanelToThreadMessageEvent()
    , mThreadToPanelMessageQueue()
    , mThreadToPanelMessageQueueMutex()
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetScrollRate(5, 5);

    // Ensure one tile always fits, accounting for the V scrollbar
    SetMinSize(wxSize(MinPreviewWidth + 20, -1));

    Bind(wxEVT_SIZE, &ShipPreviewPanel::OnResized, this, this->GetId());

    // Setup poll queue timer
    mPollQueueTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Bind(wxEVT_TIMER, &ShipPreviewPanel::OnPollQueueTimer, this, mPollQueueTimer->GetId());

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

    // Clear message queue
    assert(mThreadToPanelMessageQueue.empty());
    mThreadToPanelMessageQueue.clear(); // You never know there's another path that leads to Open() without going through Close()

    // Start thread
    assert(!mPreviewThread.joinable());
    mPreviewThread = std::thread(&ShipPreviewPanel::RunPreviewThread, this);

    // Start queue poll timer
    mPollQueueTimer->Start(10, false);
}

void ShipPreviewPanel::OnClose()
{
    // Stop queue poll timer
    mPollQueueTimer->Stop();

    // Stop thread
    assert(mPreviewThread.joinable());
    ShutdownPreviewThread();

    // Clear message queue
    mThreadToPanelMessageQueue.clear();


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

void ShipPreviewPanel::Search(std::string const & shipName)
{
    if (!mPreviewPanelSizer || shipName.empty())
        return;

    std::string const shipNameLCase = Utils::ToLower(shipName);

    //
    // Find first ship that contains the requested name as a substring
    //

    std::optional<size_t> foundShipIndex;
    for (size_t s = 0; s < mShipNameToPreviewIndex.size(); ++s)
    {
        if (mShipNameToPreviewIndex[s].find(shipNameLCase) != std::string::npos)
        {
            foundShipIndex = s;
            break;
        }
    }

    if (!!foundShipIndex)
    {
        //
        // Scroll to the item
        //

        assert(*foundShipIndex < mPreviewPanelSizer->GetItemCount());

        auto item = mPreviewPanelSizer->GetItem(*foundShipIndex);
        if (nullptr != item)
        {
            int xUnit, yUnit;
            GetScrollPixelsPerUnit(&xUnit, &yUnit);
            if (yUnit != 0)
            {
                this->Scroll(
                    -1,
                    item->GetPosition().y / yUnit);
            }
        }

        //
        // Select the item
        //

        assert(*foundShipIndex < mPreviewControls.size());

        mPreviewControls[*foundShipIndex]->Select();
    }
}

void ShipPreviewPanel::ChooseSearched()
{
    if (!!mSelectedPreview)
    {
        assert(*mSelectedPreview < mPreviewControls.size());

        mPreviewControls[*mSelectedPreview]->Choose();
    }
}

void ShipPreviewPanel::OnResized(wxSizeEvent & event)
{
    LogMessage("ShipPreviewPanel::OnResized(", event.GetSize().GetWidth(), ", ", event.GetSize().GetHeight(), "): processing...");

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

            LogMessage("ShipPreviewPanel::OnResized: rearranging...");

            Freeze();

            mPreviewPanelSizer->SetCols(nCols);
            mPreviewPanel->Layout();
            mPreviewPanelSizer->SetSizeHints(mPreviewPanel);
            this->Refresh();

            Thaw();

            LogMessage("ShipPreviewPanel::OnResized: ...rearranged.");
        }
    }

    event.Skip();

    LogMessage("ShipPreviewPanel::OnResized: ...processing completed.");
}

void ShipPreviewPanel::OnPollQueueTimer(wxTimerEvent & /*event*/)
{
    // Lock queue
    std::scoped_lock lock(mThreadToPanelMessageQueueMutex);

    // Process these many messages at a time
    for (size_t i = 0; i < 2 && !mThreadToPanelMessageQueue.empty(); ++i)
    {
        auto message = std::move(mThreadToPanelMessageQueue.front());
        mThreadToPanelMessageQueue.pop_front();

        switch (message->GetMessageType())
        {
            case ThreadToPanelMessage::MessageType::DirScanCompleted:
            {
                OnDirScanCompleted(message->GetScannedShipFilepaths());

                break;
            }

            case ThreadToPanelMessage::MessageType::DirScanError:
            {
                throw GameException(message->GetErrorMessage());
            }

            case ThreadToPanelMessage::MessageType::PreviewReady:
            {
                assert(message->GetShipIndex() < mPreviewControls.size());
                mPreviewControls[message->GetShipIndex()]->SetPreviewContent(message->GetShipPreview());

                break;
            }

            case ThreadToPanelMessage::MessageType::PreviewError:
            {
                assert(message->GetShipIndex() < mPreviewControls.size());
                mPreviewControls[message->GetShipIndex()]->SetPreviewContent(mErrorImage, message->GetErrorMessage(), "");

                break;
            }

            case ThreadToPanelMessage::MessageType::PreviewCompleted:
            {
                LogMessage("ShipPreviewPanel::OnPollQueueTimer: PreviewCompleted for ", message->GetScannedDirectoryPath().string());

                // Remember the current directory, now that it's complete
                mCurrentlyCompletedDirectory = message->GetScannedDirectoryPath();

                break;
            }
        }
    }
}

void ShipPreviewPanel::OnShipFileSelected(fsShipFileSelectedEvent & event)
{
    LogMessage("ShipPreviewPanel::OnShipFileSelected(): processing...");

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

    LogMessage("ShipPreviewPanel::OnShipFileSelected(): ...processing completed.");
}

/////////////////////////////////////////////////////////////////////////////////

void ShipPreviewPanel::OnDirScanCompleted(std::vector<std::filesystem::path> const & scannedShipFilepaths)
{
    //
    // Create new panel
    //

    wxPanel * newPreviewPanel = new wxPanel();
    newPreviewPanel->Hide();
    newPreviewPanel->Create(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxGridSizer * newPreviewPanelSizer = nullptr;

    std::vector<ShipPreviewControl *> newPreviewControls;
    std::vector<std::string> newShipNameToPreviewIndex;

    if (!scannedShipFilepaths.empty())
    {

        //
        // Create new preview controls
        //

        newPreviewPanelSizer = new wxGridSizer(CalculateTileColumns(), 0, 0);

        for (size_t shipIndex = 0; shipIndex < scannedShipFilepaths.size(); ++shipIndex)
        {
            auto shipPreviewControl = new ShipPreviewControl(
                newPreviewPanel,
                shipIndex,
                scannedShipFilepaths[shipIndex],
                PreviewVGap,
                mWaitImage,
                mErrorImage);

            newPreviewControls.push_back(shipPreviewControl);

            // Register for preview selections
            shipPreviewControl->Bind(fsEVT_SHIP_FILE_SELECTED, &ShipPreviewPanel::OnShipFileSelected, this);

            // Add to sizer
            newPreviewPanelSizer->Add(shipPreviewControl, 0, wxALIGN_CENTRE_HORIZONTAL | wxALIGN_TOP);

            // Populate name->index map for search
            newShipNameToPreviewIndex.push_back(Utils::ToLower(scannedShipFilepaths[shipIndex].filename().string()));
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
    mShipNameToPreviewIndex = newShipNameToPreviewIndex;

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


    //
    // Refresh scroll bar
    //

    this->Scroll(-1, 0);

    this->FitInside();
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
            // Scan directory
            //

            try
            {
                ScanDirectory(message->GetDirectoryPath());
            }
            catch (std::exception const & ex)
            {
                // Send error message
                QueueThreadToPanelMessage(
                    ThreadToPanelMessage::MakeDirScanErrorMessage(
                        ex.what()));
            }
        }
    }

    LogMessage("PreviewThread::Exit");
}

void ShipPreviewPanel::ScanDirectory(std::filesystem::path const & directoryPath)
{
    LogMessage("PreviewThread::ScanDirectory(", directoryPath.string(), "): processing...");


    //
    // Get listings and fire event
    //

    LogMessage("PreviewThread::ScanDirectory(): scanning directory...");

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

    LogMessage("PreviewThread::ScanDirectory(): ...directory scanned.");

    // Sort by filename
    std::sort(
        shipFilepaths.begin(),
        shipFilepaths.end(),
        [](auto const & a, auto const & b) -> bool
        {
            return a.filename().string() < b.filename().string();
        });

    // Notify
    QueueThreadToPanelMessage(
        ThreadToPanelMessage::MakeDirScanCompletedMessage(
            shipFilepaths));


    //
    // Process all files and create previews
    //

    for (size_t iShip = 0; iShip < shipFilepaths.size(); ++iShip)
    {
        // Check whether we have been interrupted
        if (!!mPanelToThreadMessage)
        {
            LogMessage("PreviewThread::ScanDirectory(): interrupted, exiting");
            return;
        }

        try
        {
            LogMessage("PreviewThread::ScanDirectory(): loading preview for \"", shipFilepaths[iShip].filename().string(), "\"...");

            // Load preview
            auto shipPreview = ShipPreview::Load(
                shipFilepaths[iShip],
                ImageSize(ShipPreviewControl::ImageWidth, ShipPreviewControl::ImageHeight));

            LogMessage("PreviewThread::ScanDirectory(): ...preview loaded.");

            // Notify
            QueueThreadToPanelMessage(
                ThreadToPanelMessage::MakePreviewReadyMessage(
                    iShip,
                    std::move(shipPreview)));
        }
        catch (std::exception const & ex)
        {
            LogMessage("PreviewThread::ScanDirectory(): encountered error, notifying...");

            // Notify
            QueueThreadToPanelMessage(
                ThreadToPanelMessage::MakePreviewErrorMessage(
                    iShip,
                    ex.what()));

            LogMessage("PreviewThread::ScanDirectory(): ...error notified.");
        }
    }


    //
    // Notify completion
    //

    QueueThreadToPanelMessage(
        ThreadToPanelMessage::MakePreviewCompletedMessage(
            directoryPath));

    LogMessage("PreviewThread::ScanDirectory(): ...preview completed.");
}

void ShipPreviewPanel::QueueThreadToPanelMessage(std::unique_ptr<ThreadToPanelMessage> message)
{
    // Lock queue
    std::scoped_lock lock(mThreadToPanelMessageQueueMutex);

    // Push message
    mThreadToPanelMessageQueue.push_back(std::move(message));
}