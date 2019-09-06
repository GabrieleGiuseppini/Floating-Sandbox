/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-09-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewPanel2.h"

#include "WxHelpers.h"

#include <Game/ImageFileTools.h>
#include <Game/ShipDefinitionFile.h>

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

ShipPreviewPanel2::ShipPreviewPanel2(
    wxWindow* parent,
    ResourceLoader const & resourceLoader)
    : wxScrolled<wxPanel>(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE | wxVSCROLL)
    , mClientSize(0, 0)
    , mVirtualHeight(0)
    , mCols(0)
    , mRows(0)
    , mExpandedHorizontalMargin(0)
    , mWaitBitmap(WxHelpers::MakeBitmap(ImageFileTools::LoadImageRgbaLowerLeft(resourceLoader.GetBitmapFilepath("ship_preview_wait"))))
    , mErrorBitmap(WxHelpers::MakeBitmap(ImageFileTools::LoadImageRgbaLowerLeft(resourceLoader.GetBitmapFilepath("ship_preview_error"))))
    //
    , mPollQueueTimer()
    , mInfoTiles()
    , mShipNameToInfoTileIndex()
    , mSelectedInfoTileIndex()
    , mCurrentlyCompletedDirectory()
    //
    , mPreviewThread()
    , mPanelToThreadMessage()
    , mPanelToThreadMessageMutex()
    , mPanelToThreadMessageLock(mPanelToThreadMessageMutex, std::defer_lock)
    , mPanelToThreadMessageEvent()
    , mThreadToPanelMessageQueue()
    , mThreadToPanelMessageQueueMutex()
{
    SetScrollRate(5, 5);

    // Initialize rendering
    SetDoubleBuffered(true);
    SetBackgroundColour(wxColour("WHITE"));
    mSelectionPen = wxPen(wxColor(0x00, 0x00, 0x00), 1, wxPENSTYLE_SOLID);
    mDescriptionFont = wxFont(wxFontInfo(7));
    mFilenameFont = wxFont(wxFontInfo(7).Italic());

    // Ensure one tile always fits, accounting for the V scrollbar
    SetMinSize(wxSize(PanelWidthMin + 20, -1));

    // Register paint and resize
    Bind(wxEVT_PAINT, &ShipPreviewPanel2::OnPaint, this, this->GetId());
    Bind(wxEVT_SIZE, &ShipPreviewPanel2::OnResized, this, this->GetId());

    // Setup poll queue timer
    mPollQueueTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Bind(wxEVT_TIMER, &ShipPreviewPanel2::OnPollQueueTimer, this, mPollQueueTimer->GetId());
}

ShipPreviewPanel2::~ShipPreviewPanel2()
{
    // Stop thread
    if (mPreviewThread.joinable())
    {
        ShutdownPreviewThread();
    }
}

void ShipPreviewPanel2::OnOpen()
{
    assert(!mSelectedInfoTileIndex);

    // Clear message queue
    assert(mThreadToPanelMessageQueue.empty());
    mThreadToPanelMessageQueue.clear(); // You never know there's another path that leads to Open() without going through Close()

    // Start thread
    assert(!mPreviewThread.joinable());
    mPreviewThread = std::thread(&ShipPreviewPanel2::RunPreviewThread, this);

    // Start queue poll timer
    mPollQueueTimer->Start(50, false);
}

void ShipPreviewPanel2::OnClose()
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

    mSelectedInfoTileIndex.reset();
}

void ShipPreviewPanel2::SetDirectory(std::filesystem::path const & directoryPath)
{
    // Check if different than current
    if (directoryPath != mCurrentlyCompletedDirectory)
    {
        //
        // Change directory
        //

        mCurrentlyCompletedDirectory.reset();

        // Clear state
        mInfoTiles.clear();
        mSelectedInfoTileIndex.reset();

        // Tell thread (if it's running)
        mPanelToThreadMessageLock.lock();
        mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeSetDirectoryMessage(directoryPath)));
        mPanelToThreadMessageEvent.notify_one();
        mPanelToThreadMessageLock.unlock();
    }
}

void ShipPreviewPanel2::Search(std::string const & shipName)
{
    // TODO
    ////if (!mPreviewPanelSizer || shipName.empty())
    ////    return;

    ////std::string const shipNameLCase = Utils::ToLower(shipName);

    //////
    ////// Find first ship that contains the requested name as a substring
    //////

    ////std::optional<size_t> foundShipIndex;
    ////for (size_t s = 0; s < mShipNameToPreviewIndex.size(); ++s)
    ////{
    ////    if (mShipNameToPreviewIndex[s].find(shipNameLCase) != std::string::npos)
    ////    {
    ////        foundShipIndex = s;
    ////        break;
    ////    }
    ////}

    ////if (!!foundShipIndex)
    ////{
    ////    //
    ////    // Scroll to the item
    ////    //

    ////    assert(*foundShipIndex < mPreviewPanelSizer->GetItemCount());

    ////    auto item = mPreviewPanelSizer->GetItem(*foundShipIndex);
    ////    if (nullptr != item)
    ////    {
    ////        int xUnit, yUnit;
    ////        GetScrollPixelsPerUnit(&xUnit, &yUnit);
    ////        if (yUnit != 0)
    ////        {
    ////            this->Scroll(
    ////                -1,
    ////                item->GetPosition().y / yUnit);
    ////        }
    ////    }

    ////    //
    ////    // Select the item
    ////    //

    ////    assert(*foundShipIndex < mPreviewControls.size());

    ////    mPreviewControls[*foundShipIndex]->Select();
    ////}
}

void ShipPreviewPanel2::ChooseSearched()
{
    // TODO
    ////if (!!mSelectedPreview)
    ////{
    ////    assert(*mSelectedPreview < mPreviewControls.size());

    ////    mPreviewControls[*mSelectedPreview]->Choose();
    ////}
}

void ShipPreviewPanel2::OnPaint(wxPaintEvent & event)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ShipPreviewPanel2::OnResized(wxSizeEvent & event)
{
    auto clientSize = GetClientSize();

    LogMessage("ShipPreviewPanel::OnResized(", clientSize.GetWidth(), ", ", clientSize.GetHeight(), " (client)): processing...");

    RecalculateGeometry(
        clientSize,
        static_cast<int>(mInfoTiles.size()));

    Refresh();

    LogMessage("ShipPreviewPanel::OnResized: ...processing completed.");

    // Keep processing this event (so to redraw)
    event.Skip();
}

void ShipPreviewPanel2::OnPollQueueTimer(wxTimerEvent & /*event*/)
{
    bool doRefresh = false;

    // Process these many messages at a time
    for (size_t i = 0; i < 10; ++i)
    {
        // Poll a message
        std::unique_ptr<ThreadToPanelMessage> message;
        {
            std::scoped_lock lock(mThreadToPanelMessageQueueMutex);

            if (!mThreadToPanelMessageQueue.empty())
            {
                message = std::move(mThreadToPanelMessageQueue.front());
                mThreadToPanelMessageQueue.pop_front();
            }
        }

        if (!message)
            break; // No message found

        switch (message->GetMessageType())
        {
            case ThreadToPanelMessage::MessageType::DirScanCompleted:
            {
                LogMessage("ShipPreviewPanel::Poll: Processing DirScanCompleted...");

                assert(mInfoTiles.empty());
                mInfoTiles.reserve(message->GetScannedShipFilepaths().size());

                for (auto const & shipFilepath : message->GetScannedShipFilepaths())
                {
                    mInfoTiles.emplace_back(
                        mWaitBitmap,
                        "",
                        "",
                        shipFilepath.filename().string());
                }

                // Recalculate geometry
                RecalculateGeometry(mClientSize, static_cast<int>(mInfoTiles.size()));

                LogMessage("ShipPreviewPanel::Poll: ...DirScanCompleted processed.");

                doRefresh = true;

                break;
            }

            case ThreadToPanelMessage::MessageType::DirScanError:
            {
                throw GameException(message->GetErrorMessage());
            }

            case ThreadToPanelMessage::MessageType::PreviewReady:
            {
                LogMessage("ShipPreviewPanel::Poll: Processing preview for ", message->GetShipIndex(), "...");

                //
                // Populate info tile
                //

                assert(message->GetShipIndex() < mInfoTiles.size());

                mInfoTiles[message->GetShipIndex()].Bitmap = WxHelpers::MakeBitmap(message->GetShipPreview().PreviewImage);

                std::string descriptionLabelText1 = message->GetShipPreview().Metadata.ShipName;
                if (!!message->GetShipPreview().Metadata.YearBuilt)
                    descriptionLabelText1 += " (" + *(message->GetShipPreview().Metadata.YearBuilt) + ")";
                mInfoTiles[message->GetShipIndex()].Description1 = std::move(descriptionLabelText1);

                int metres = message->GetShipPreview().OriginalSize.Width;
                int feet = static_cast<int>(round(3.28f * metres));
                std::string descriptionLabelText2 =
                    std::to_string(metres)
                    + "m/"
                    + std::to_string(feet)
                    + "ft";
                if (!!message->GetShipPreview().Metadata.Author)
                    descriptionLabelText2 += " - by " + *(message->GetShipPreview().Metadata.Author);
                mInfoTiles[message->GetShipIndex()].Description2 = std::move(descriptionLabelText2);

                LogMessage("ShipPreviewPanel::Poll: ...preview processed.");

                doRefresh = true;

                break;
            }

            case ThreadToPanelMessage::MessageType::PreviewError:
            {
                // Set error image
                assert(message->GetShipIndex() < mInfoTiles.size());
                mInfoTiles[message->GetShipIndex()].Bitmap = mErrorBitmap;
                mInfoTiles[message->GetShipIndex()].Description1 = message->GetErrorMessage();

                doRefresh = true;

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

    if (doRefresh)
    {
        Refresh();
    }
}

void ShipPreviewPanel2::OnShipFileSelected(fsShipFileSelectedEvent & event)
{
    LogMessage("ShipPreviewPanel::OnShipFileSelected(): processing...");

    // TODO
    //////
    ////// Toggle selection
    //////

    ////if (!!mSelectedPreview)
    ////{
    ////    assert(*mSelectedPreview < mPreviewControls.size());
    ////    mPreviewControls[*mSelectedPreview]->SetSelected(false);
    ////}

    ////assert(event.GetShipIndex() < mPreviewControls.size());
    ////mPreviewControls[event.GetShipIndex()]->SetSelected(true);

    ////mSelectedPreview = event.GetShipIndex();

    // Propagate up
    ProcessWindowEvent(event);

    LogMessage("ShipPreviewPanel::OnShipFileSelected(): ...processing completed.");
}

/////////////////////////////////////////////////////////////////////////////////

void ShipPreviewPanel2::RecalculateGeometry(
    wxSize clientSize,
    int nPreviews)
{
    // Store size
    mClientSize = clientSize;

    // Calculate number of columns
    mCols = static_cast<int>(static_cast<float>(clientSize.GetWidth()) / static_cast<float>(InfoTileWidth + HorizontalMarginMin));
    assert(mCols >= 1);

    // Calculate expanded horizontal margin
    mExpandedHorizontalMargin = (clientSize.GetWidth() - mCols * InfoTileWidth) / mCols;
    assert(mExpandedHorizontalMargin >= HorizontalMarginMin);

    // TODOTEST
    LogMessage("RECALC TODOTEST: w=", clientSize.GetWidth(), " calcdW=", mCols * (InfoTileWidth + mExpandedHorizontalMargin));

    // Calculate number of rows
    mRows =
        nPreviews / mCols
        + ((nPreviews % mCols) ? 1 : 0);

    // Calculate virtual height
    mVirtualHeight = mRows * RowHeight;

    // Set virtual size
    SetVirtualSize(clientSize.GetWidth(), mVirtualHeight);

    LogMessage("ShipPreviewPanel::RecalculateGeometry(", clientSize.GetWidth(), ", ", clientSize.GetHeight(), ", ", nPreviews,"): nCols=",
        mCols, " nRows=", mRows, " expHMargin=", mExpandedHorizontalMargin, " virtH=", mVirtualHeight);

    // Update all info tiles's rectangles
    int const columnWidth = InfoTileWidth + mExpandedHorizontalMargin;
    for (int i = 0; i < mInfoTiles.size(); ++i)
    {
        mInfoTiles[i].Col = i % mCols;
        mInfoTiles[i].Row = i / mCols;

        int x = mInfoTiles[i].Col * columnWidth;
        int y = mInfoTiles[i].Row * RowHeight;
        mInfoTiles[i].RectVirtual = wxRect(x, y, columnWidth, RowHeight);
    }
}

void ShipPreviewPanel2::Render(wxDC & dc)
{
    dc.Clear();

    if (!mInfoTiles.empty())
    {
        // Calculate visible portion in virtual space
        wxRect visibleRectVirtual(GetClientSize());
        visibleRectVirtual.Offset(CalcUnscrolledPosition(visibleRectVirtual.GetTopLeft()));

        // TODOTEST
        LogMessage("VisibleRectVirtual: x=", visibleRectVirtual.GetX(), " y=", visibleRectVirtual.GetY(),
            " w=", visibleRectVirtual.GetWidth(), " h=", visibleRectVirtual.GetHeight());

        // Calculate virtual origin - all virtual coordinates will need this subtracted from them in
        // order to become device coordinates
        wxPoint originVirtual = visibleRectVirtual.GetTopLeft();

        // Calculate left margin for content of info tile
        int const infoTileContentLeftMargin = mExpandedHorizontalMargin / 2 + InfoTileInset;

        // Process all info tiles
        for (size_t i = 0; i < mInfoTiles.size(); ++i)
        {
            auto const & infoTile = mInfoTiles[i];

            // Check if this info tile's virtual rect intersects the visible one
            if (visibleRectVirtual.Intersects(infoTile.RectVirtual))
            {
                // TODOHERE

                // Draw bitmap
                dc.DrawBitmap(
                    infoTile.Bitmap,
                    infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                        + PreviewImageWidth / 2 - infoTile.Bitmap.GetWidth() / 2
                        - originVirtual.x,
                    infoTile.RectVirtual.GetTop() + InfoTileInset
                        + PreviewImageHeight - infoTile.Bitmap.GetHeight()
                        - originVirtual.y,
                    true);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////

void ShipPreviewPanel2::ShutdownPreviewThread()
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

void ShipPreviewPanel2::RunPreviewThread()
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

void ShipPreviewPanel2::ScanDirectory(std::filesystem::path const & directoryPath)
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
                ImageSize(PreviewImageWidth, PreviewImageHeight));

            LogMessage("PreviewThread::ScanDirectory(): ...preview loaded.");

            // Notify
            QueueThreadToPanelMessage(
                ThreadToPanelMessage::MakePreviewReadyMessage(
                    iShip,
                    std::move(shipPreview)));

            // Take it easy a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

void ShipPreviewPanel2::QueueThreadToPanelMessage(std::unique_ptr<ThreadToPanelMessage> message)
{
    // Lock queue
    std::scoped_lock lock(mThreadToPanelMessageQueueMutex);

    // Push message
    mThreadToPanelMessageQueue.push_back(std::move(message));
}