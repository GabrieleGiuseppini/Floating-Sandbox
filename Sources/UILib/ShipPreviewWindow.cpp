/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-09-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewWindow.h"

#include <UILib/WxHelpers.h>

#include <Game/ShipDeSerializer.h>
#include <Game/ShipPreviewDirectoryManager.h>

#include <Core/Conversions.h>
#include <Core/GameRandomEngine.h>
#include <Core/GameException.h>
#include <Core/ImageTools.h>
#include <Core/Log.h>

#include <algorithm>
#include <limits>

wxDEFINE_EVENT(fsEVT_SHIP_FILE_SELECTED, ShipPreviewWindow::fsShipFileSelectedEvent);
wxDEFINE_EVENT(fsEVT_SHIP_FILE_CHOSEN, ShipPreviewWindow::fsShipFileChosenEvent);

ShipPreviewWindow::ShipPreviewWindow(
    wxWindow* parent,
    GameAssetManager const & gameAssetManager)
    : wxScrolled<wxWindow>(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE | wxVSCROLL
        | wxWANTS_CHARS // To catch ENTER key
        )
    , mClientSize(0, 0)
    , mVirtualHeight(0)
    , mCols(0)
    , mRows(0)
    , mColumnWidth(0)
    , mExpandedHorizontalMargin(0)
    , mWaitBitmap(WxHelpers::MakeBitmap(GameAssetManager::LoadPngImageRgba(gameAssetManager.GetBitmapFilePath("ship_preview_wait"))))
    , mErrorBitmap(WxHelpers::MakeBitmap(GameAssetManager::LoadPngImageRgba(gameAssetManager.GetBitmapFilePath("ship_preview_error"))))
    , mPreviewRibbonBatteryBitmap(WxHelpers::MakeBitmap(GameAssetManager::LoadPngImageRgba(gameAssetManager.GetBitmapFilePath("ship_preview_ribbon_battery"))))
    , mPreviewRibbonHDBitmap(WxHelpers::MakeBitmap(GameAssetManager::LoadPngImageRgba(gameAssetManager.GetBitmapFilePath("ship_preview_ribbon_hd"))))
    , mPreviewRibbonBatteryAndHDBitmap(WxHelpers::MakeBitmap(GameAssetManager::LoadPngImageRgba(gameAssetManager.GetBitmapFilePath("ship_preview_ribbon_battery_and_hd"))))
    //
    , mPollQueueTimer()
    , mInfoTiles()
    , mSelectedShipFileId()
    , mSortMethod(SortMethod::ByName)
    , mIsSortDescending(false)
    , mSortPredicate(MakeSortPredicate(mSortMethod, mIsSortDescending))
    , mCurrentlyCompletedDirectorySnapshot()
    //
    , mPreviewThread()
    , mPanelToThreadMessage()
    , mPanelToThreadMessageMutex()
    , mPanelToThreadMessageEvent()
    , mThreadToPanelMessageQueue()
    , mThreadToPanelMessageQueueMutex()
    , mThreadToPanelScanInterruptAck(false)
    , mThreadToPanelScanInterruptAckMutex()
    , mThreadToPanelScanInterruptAckEvent()
{
    SetScrollRate(0, 20);

    // Initialize rendering
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif
    SetBackgroundColour(wxColour("WHITE"));
    mSelectionPen = wxPen(wxColor(0x10, 0x10, 0x10), 1, wxPENSTYLE_SOLID);
    mDescriptionFont = wxFont(wxFontInfo(7));
    mDescriptionColor = wxColor(0, 0, 0);
    mFilenameFont = wxFont(wxFontInfo(6).Italic());
    mFilenameColor = wxColor(40, 40, 40);

    // Ensure one tile always fits, accounting for the V scrollbar
    SetMinSize(wxSize(PanelWidthMin + 20, -1));

    // Register paint and resize
    Bind(wxEVT_PAINT, &ShipPreviewWindow::OnPaint, this);
    Bind(wxEVT_SIZE, &ShipPreviewWindow::OnResized, this);

    // Register mouse events
    Bind(wxEVT_LEFT_DOWN, &ShipPreviewWindow::OnMouseSingleClick, this);
    Bind(wxEVT_LEFT_DCLICK, &ShipPreviewWindow::OnMouseDoubleClick, this);

    // Register key events
    Bind(wxEVT_KEY_DOWN, &ShipPreviewWindow::OnKeyDown, this);

    // Setup poll queue timer
    mPollQueueTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Bind(wxEVT_TIMER, &ShipPreviewWindow::OnPollQueueTimer, this, mPollQueueTimer->GetId());
}

ShipPreviewWindow::~ShipPreviewWindow()
{
    // Stop thread
    if (mPreviewThread.joinable())
    {
        ShutdownPreviewThread();
    }
}

void ShipPreviewWindow::OnOpen()
{
    assert(!mSelectedShipFileId);

    // Clear message queue
    assert(mThreadToPanelMessageQueue.empty());
    mThreadToPanelMessageQueue.clear(); // You never know there's another path that leads to Open() without going through Close()

    // Start thread
    LogMessage("ShipPreviewWindow::OnOpen: Starting thread");
    assert(!mPreviewThread.joinable());
    mPreviewThread = std::thread(&ShipPreviewWindow::RunPreviewThread, this);

    // Start queue poll timer
    mPollQueueTimer->Start(25, false);
}

void ShipPreviewWindow::OnClose()
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

    mSelectedShipFileId.reset();
}

void ShipPreviewWindow::SetDirectory(std::filesystem::path const & directoryPath)
{
    LogMessage("ShipPreviewWindow::SetDirectory(", directoryPath.string(), ")");

    //
    // Build set of files from directory
    //

    DirectorySnapshot directorySnapshot = EnumerateShipFiles(directoryPath);

    // Check if we're moving to a new directory, or if not, if there's
    // a change in the current directory
    if (!mCurrentlyCompletedDirectorySnapshot.has_value()
        || !mCurrentlyCompletedDirectorySnapshot->IsEquivalentTo(directorySnapshot))
    {
        LogMessage("ShipPreviewWindow::SetDirectory(", directoryPath.string(), "): change detected");

        //
        // Stop thread's scan (if thread is running)
        //

        if (mPreviewThread.joinable())
        {
            // Send InterruptScan
            {
                std::lock_guard<std::mutex> lock(mPanelToThreadMessageMutex);

                mThreadToPanelScanInterruptAck = false;
                mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeInterruptScanMessage()));
                mPanelToThreadMessageEvent.notify_one();
            }

            // Wait for ack
            {
                std::unique_lock<std::mutex> lock(mThreadToPanelScanInterruptAckMutex, std::defer_lock);
                lock.lock();

                mThreadToPanelScanInterruptAckEvent.wait(
                    lock,
                    [this]()
                    {
                        return mThreadToPanelScanInterruptAck;
                    });

                lock.unlock();
            }

            // Clear message queue
            // Note: no need to lock as we know the thread is not touching it
            mThreadToPanelMessageQueue.clear();
        }

        //
        // Change directory
        //

        mCurrentlyCompletedDirectorySnapshot.reset();

        // Clear selection
        mSelectedShipFileId.reset();

        // Reset info tiles
        ResetInfoTiles(directorySnapshot);

        // Start thread's scan (if thread is not running now, it'll pick it up when it starts)
        {
            std::lock_guard<std::mutex> lock(mPanelToThreadMessageMutex);

            mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeSetDirectoryMessage(std::move(directorySnapshot))));
            mPanelToThreadMessageEvent.notify_one();
        }
    }
    else
    {
        LogMessage("ShipPreviewWindow::SetDirectory(", directoryPath.string(), "): no change detected");
    }
}

bool ShipPreviewWindow::Search(std::string const & shipName)
{
    assert(!shipName.empty());

    std::string const shipNameLCase = Utils::ToLower(shipName);

    //
    // Find next ship that contains the requested name as a substring,
    // doing a circular search from the currently-selected ship
    //

    std::optional<size_t> foundShipIndex;
    size_t startInfoTileIndex = mSelectedShipFileId ? (ShipFileIdToInfoTileIndex(*mSelectedShipFileId) + 1) : 0;
    for (size_t i = 0; i < mInfoTiles.size(); ++i)
    {
        size_t const shipInfoTileIndex = (startInfoTileIndex + i) % mInfoTiles.size();

        if (std::any_of(
            mInfoTiles[shipInfoTileIndex].SearchStrings.cbegin(),
            mInfoTiles[shipInfoTileIndex].SearchStrings.cend(),
            [&shipNameLCase](auto const & str)
            {
                return str.find(shipNameLCase) != std::string::npos;
            }))
        {
            foundShipIndex = shipInfoTileIndex;
            break;
        }
    }

    if (foundShipIndex.has_value())
    {
        //
        // Scroll to the item if it's not fully visible
        //

        EnsureInfoTileIsVisible(*foundShipIndex);

        //
        // Select item
        //

        SelectInfoTile(*foundShipIndex);
    }

    return foundShipIndex.has_value();
}

void ShipPreviewWindow::SetSortMethod(SortMethod sortMethod)
{
    mSortMethod = sortMethod;
    RefreshSortPredicate();
    SortInfoTiles();
    Refresh();
}

void ShipPreviewWindow::SetIsSortDescending(bool isSortDescending)
{
    mIsSortDescending = isSortDescending;
    RefreshSortPredicate();
    SortInfoTiles();
    Refresh();
}

std::optional<std::filesystem::path> ShipPreviewWindow::ChooseShipRandomly(std::optional<std::filesystem::path> currentShip) const
{
    if (mInfoTiles.size() > 0 && (!currentShip.has_value() || mInfoTiles.size() > 1))
    {
        for (int t = 0; t < 5; ++t) // Safety
        {
            size_t chosen = GameRandomEngine::GetInstance().Choose(mInfoTiles.size());
            auto const shipFilepath = mInfoTiles[chosen].ShipFilepath;
            if (!currentShip.has_value() || *currentShip != shipFilepath)
            {
                return shipFilepath;
            }
        }
    }

    // No luck - return current ship if just one exists
    return currentShip;
}

void ShipPreviewWindow::ChooseSelectedIfAny()
{
    if (mSelectedShipFileId.has_value())
    {
        ChooseInfoTile(ShipFileIdToInfoTileIndex(*mSelectedShipFileId));
    }
}

void ShipPreviewWindow::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(this);
    Render(dc);
}

void ShipPreviewWindow::OnResized(wxSizeEvent & event)
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

void ShipPreviewWindow::OnMouseSingleClick(wxMouseEvent & event)
{
    auto selectedInfoTileIndex = MapMousePositionToInfoTile(event.GetPosition());
    if (selectedInfoTileIndex < mInfoTiles.size())
    {
        SelectInfoTile(selectedInfoTileIndex);
    }

    // Allow focus move
    event.Skip();
}

void ShipPreviewWindow::OnMouseDoubleClick(wxMouseEvent & event)
{
    auto selectedInfoTileIndex = MapMousePositionToInfoTile(event.GetPosition());
    if (selectedInfoTileIndex < mInfoTiles.size())
    {
        ChooseInfoTile(selectedInfoTileIndex);
    }
}

void ShipPreviewWindow::OnKeyDown(wxKeyEvent & event)
{
    if (!mSelectedShipFileId.has_value())
    {
        event.Skip();
        return;
    }

    int deltaElement = 0;

    auto const keyCode = event.GetKeyCode();
    if (keyCode == WXK_LEFT)
        deltaElement = -1;
    else if (keyCode == WXK_RIGHT)
        deltaElement = 1;
    else if (keyCode == WXK_UP)
        deltaElement = -mCols;
    else if (keyCode == WXK_DOWN)
        deltaElement = mCols;
    else if (keyCode == WXK_RETURN)
    {
        ChooseInfoTile(ShipFileIdToInfoTileIndex(*mSelectedShipFileId));
        return;
    }
    else
    {
        event.Skip();
        return;
    }

    if (deltaElement != 0)
    {
        int const newInfoTileIndex = static_cast<int>(ShipFileIdToInfoTileIndex(*mSelectedShipFileId)) + deltaElement;
        if (newInfoTileIndex >= 0 && newInfoTileIndex < static_cast<int>(mInfoTiles.size()))
        {
            SelectInfoTile(newInfoTileIndex);

            // Move into view if needed
            EnsureInfoTileIsVisible(newInfoTileIndex);
        }
    }
}

void ShipPreviewWindow::OnPollQueueTimer(wxTimerEvent & /*event*/)
{
    bool haveInfoTilesBeenUpdated = false;

    // Process max these many messages at a time
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
            case ThreadToPanelMessage::MessageType::DirScanError:
            {
                throw GameException(message->GetErrorMessage());
            }

            case ThreadToPanelMessage::MessageType::PreviewReady:
            {
                //
                // Populate info tile
                //

                size_t const infoTileIndex = ShipFileIdToInfoTileIndex(message->GetShipFileId());
                assert(infoTileIndex < mInfoTiles.size());

                auto & infoTile = mInfoTiles[infoTileIndex];

                EnhancedShipPreviewData const & shipPreviewData = message->GetShipPreviewData();

                infoTile.Bitmap = MakeBitmap(message->GetShipPreviewImage());
                infoTile.IsHD = shipPreviewData.IsHD;
                infoTile.HasElectricals = shipPreviewData.HasElectricals;

                infoTile.FeatureScore = 0;
                if (shipPreviewData.IsHD)
                    infoTile.FeatureScore += 1;
                if (shipPreviewData.HasElectricals)
                    infoTile.FeatureScore += 2;

                infoTile.LastWriteTime = shipPreviewData.LastWriteTime;

                std::string descriptionLabelText1 = shipPreviewData.Metadata.ShipName;
                if (shipPreviewData.Metadata.YearBuilt.has_value())
                    descriptionLabelText1 += " (" + *(shipPreviewData.Metadata.YearBuilt) + ")";
                infoTile.OriginalDescription1 = std::move(descriptionLabelText1);
                infoTile.DescriptionLabel1Size.reset();

                int const metres = shipPreviewData.ShipSize.width;
                int const feet = static_cast<int>(std::round(Conversions::MeterToFoot(metres)));
                std::string descriptionLabelText2 =
                    std::to_string(metres)
                    + "m/"
                    + std::to_string(feet)
                    + "ft";
                if (shipPreviewData.Metadata.Author.has_value())
                    descriptionLabelText2 += " - by " + *(shipPreviewData.Metadata.Author);
                infoTile.OriginalDescription2 = std::move(descriptionLabelText2);
                infoTile.DescriptionLabel2Size.reset();

                if (shipPreviewData.Metadata.ArtCredits.has_value())
                    infoTile.OriginalDescription3 = "Art by " + *(shipPreviewData.Metadata.ArtCredits);
                infoTile.DescriptionLabel3Size.reset();

                infoTile.Metadata.emplace(shipPreviewData.Metadata);

                // Add ship name to search map
                infoTile.SearchStrings.push_back(
                    Utils::ToLower(
                        shipPreviewData.Metadata.ShipName));

                // Add author to search map
                if (shipPreviewData.Metadata.Author.has_value())
                {
                    infoTile.SearchStrings.push_back(
                        Utils::ToLower(
                            *(shipPreviewData.Metadata.Author)));
                }

                // Add art credits to search map
                if (shipPreviewData.Metadata.ArtCredits.has_value())
                {
                    infoTile.SearchStrings.push_back(
                        Utils::ToLower(
                            *(shipPreviewData.Metadata.ArtCredits)));
                }

                // Add ship year to search map
                if (shipPreviewData.Metadata.YearBuilt.has_value())
                {
                    infoTile.SearchStrings.push_back(
                        Utils::ToLower(
                            *(shipPreviewData.Metadata.YearBuilt)));
                }

                // Re-sort this info tile
                ResortInfoTile(infoTileIndex);

                // Remember we need to refresh now
                haveInfoTilesBeenUpdated = true;

                break;
            }

            case ThreadToPanelMessage::MessageType::PreviewError:
            {
                //
                // Set error image
                //

                size_t const infoTileIndex = ShipFileIdToInfoTileIndex(message->GetShipFileId());
                assert(infoTileIndex < mInfoTiles.size());

                mInfoTiles[infoTileIndex].Bitmap = mErrorBitmap;
                mInfoTiles[infoTileIndex].OriginalDescription1 = message->GetErrorMessage();
                mInfoTiles[infoTileIndex].DescriptionLabel1Size.reset();

                // Re-sort this info tile
                ResortInfoTile(infoTileIndex);

                // Remember we need to refresh now
                haveInfoTilesBeenUpdated = true;

                break;
            }

            case ThreadToPanelMessage::MessageType::PreviewCompleted:
            {
                LogMessage("ShipPreviewPanel::OnPollQueueTimer: PreviewCompleted for ", message->GetDirectorySnapshot().DirectoryPath.string());

                // Remember the current snapshot, now that it's complete
                mCurrentlyCompletedDirectorySnapshot.emplace(message->GetDirectorySnapshot());

                break;
            }
        }
    }

    if (haveInfoTilesBeenUpdated)
    {
        Refresh();

        EnsureSelectedShipIsVisible();
    }
}

/////////////////////////////////////////////////////////////////////////////////

void ShipPreviewWindow::SelectInfoTile(size_t infoTileIndex)
{
    bool isDirty = (mSelectedShipFileId != mInfoTiles[infoTileIndex].ShipFileId);

    mSelectedShipFileId = mInfoTiles[infoTileIndex].ShipFileId;

    if (isDirty)
    {
        // Draw selection
        Refresh();

        //
        // Fire selected event
        //

        auto event = fsShipFileSelectedEvent(
            fsEVT_SHIP_FILE_SELECTED,
            this->GetId(),
            mInfoTiles[infoTileIndex].Metadata,
            mInfoTiles[infoTileIndex].ShipFilepath);

        ProcessWindowEvent(event);
    }
}

void ShipPreviewWindow::ChooseInfoTile(size_t infoTileIndex)
{
    //
    // Fire chosen event
    //

    auto event = fsShipFileChosenEvent(
        fsEVT_SHIP_FILE_CHOSEN,
        this->GetId(),
        mInfoTiles[infoTileIndex].ShipFilepath);

    ProcessWindowEvent(event);
}

void ShipPreviewWindow::ResetInfoTiles(DirectorySnapshot const & directorySnapshot)
{
    LogMessage("ShipPreviewPanel::ResetInfoTiles start...");

    mInfoTiles.clear();
    mInfoTiles.reserve(directorySnapshot.FileEntries.size());

    for (auto const & fileEntry : directorySnapshot.FileEntries)
    {
        mInfoTiles.emplace_back(
            fileEntry.ShipFileId,
            fileEntry.FilePath,
            mWaitBitmap);

        // Add ship filename to search map
        mInfoTiles.back().SearchStrings.push_back(
            Utils::ToLower(
                fileEntry.FilePath.filename().string()));
    }

    // Sort info tiles according to current sort method
    SortInfoTiles();

    // Recalculate geometry
    RecalculateGeometry(mClientSize, static_cast<int>(mInfoTiles.size()));

    LogMessage("ShipPreviewPanel::ResetInfoTiles ...end.");

    Refresh();
}

void ShipPreviewWindow::SortInfoTiles()
{
    std::sort(
        mInfoTiles.begin(),
        mInfoTiles.end(),
        mSortPredicate);

    EnsureSelectedShipIsVisible();
}

void ShipPreviewWindow::ResortInfoTile(size_t infoTileIndex)
{
    // Extract item
    InfoTile infoTile = std::move(mInfoTiles[infoTileIndex]);
    mInfoTiles.erase(mInfoTiles.cbegin() + infoTileIndex);

    // Find position
    auto const it = std::upper_bound(
        mInfoTiles.cbegin(),
        mInfoTiles.cend(),
        infoTile,
        mSortPredicate);

    // Insert
    mInfoTiles.insert(it, std::move(infoTile));
}

size_t ShipPreviewWindow::ShipFileIdToInfoTileIndex(ShipFileId_t shipFileId) const
{
    // Search for info tile with this ship file ID
    for (size_t i = 0; i < mInfoTiles.size(); ++i)
    {
        if (mInfoTiles[i].ShipFileId == shipFileId)
        {
            return i;
        }
    }

    assert(false);
    return std::numeric_limits<size_t>::max();
}

wxRect ShipPreviewWindow::InfoTileIndexToRectVirtual(size_t infoTileIndex) const
{
    int const iCol = static_cast<int>(infoTileIndex % mCols);
    int const iRow = static_cast<int>(infoTileIndex / mCols);

    int const x = iCol * mColumnWidth;
    int const y = iRow * RowHeight;
    return wxRect(x, y, mColumnWidth, RowHeight);
}

void ShipPreviewWindow::RefreshSortPredicate()
{
    mSortPredicate = MakeSortPredicate(mSortMethod, mIsSortDescending);
}

std::function<bool(ShipPreviewWindow::InfoTile const &, ShipPreviewWindow::InfoTile const &)> ShipPreviewWindow::MakeSortPredicate(SortMethod sortMethod, bool isSortDescending)
{
    std::function<bool(InfoTile const &, InfoTile const &)> metadataPredicate;

    switch (sortMethod)
    {
        case SortMethod::ByFeatures:
        {
            metadataPredicate = [isSortDescending](InfoTile const & l, InfoTile const & r) -> bool
            {
                assert(l.Metadata.has_value() && r.Metadata.has_value());

                if (l.FeatureScore > r.FeatureScore) // We want highest score to be at top
                {
                    return (true) != (isSortDescending);
                }
                else if (l.FeatureScore == r.FeatureScore)
                {
                    auto const lShipNameI = Utils::ToLower(l.Metadata->ShipName);
                    auto const rShipNameI = Utils::ToLower(r.Metadata->ShipName);

                    return
                        (lShipNameI < rShipNameI)
                        || ((lShipNameI == rShipNameI) && (l.ShipFileId < r.ShipFileId));
                }
                else
                {
                    return (false) != (isSortDescending);
                }
            };

            break;
        }

        case SortMethod::ByLastModified:
        {
            metadataPredicate = [isSortDescending](InfoTile const & l, InfoTile const & r) -> bool
            {
                assert(l.Metadata.has_value() && r.Metadata.has_value());

                if (l.LastWriteTime > r.LastWriteTime) // We want most recent at top
                {
                    return (true) != (isSortDescending);
                }
                else if (l.LastWriteTime == r.LastWriteTime)
                {
                    auto const lShipNameI = Utils::ToLower(l.Metadata->ShipName);
                    auto const rShipNameI = Utils::ToLower(r.Metadata->ShipName);

                    return
                        (lShipNameI < rShipNameI)
                        || ((lShipNameI == rShipNameI) && (l.ShipFileId < r.ShipFileId));
                }
                else
                {
                    return (false) != (isSortDescending);
                }
            };

            break;
        }

        case SortMethod::ByName:
        {
            metadataPredicate = [isSortDescending](InfoTile const & l, InfoTile const & r) -> bool
            {
                assert(l.Metadata.has_value() && r.Metadata.has_value());

                auto const lShipNameI = Utils::ToLower(l.Metadata->ShipName);
                auto const rShipNameI = Utils::ToLower(r.Metadata->ShipName);

                bool const ascendingResult =
                    (lShipNameI < rShipNameI)
                    || ((lShipNameI == rShipNameI) && (l.ShipFileId < r.ShipFileId));

                return (ascendingResult) != (isSortDescending);
            };

            break;
        }

        case SortMethod::ByYearBuilt:
        {
            metadataPredicate = [isSortDescending](InfoTile const & l, InfoTile const & r) -> bool
            {
                assert(l.Metadata.has_value() && r.Metadata.has_value());

                if (l.Metadata->YearBuilt.has_value()
                    && r.Metadata->YearBuilt.has_value()
                    && *(l.Metadata->YearBuilt) != *(r.Metadata->YearBuilt))
                {
                    return (*(l.Metadata->YearBuilt) < *(r.Metadata->YearBuilt)) != (isSortDescending);
                }
                else if (l.Metadata->YearBuilt == r.Metadata->YearBuilt) // Either both are set and match values, or neither is set
                {
                    auto const lShipNameI = Utils::ToLower(l.Metadata->ShipName);
                    auto const rShipNameI = Utils::ToLower(r.Metadata->ShipName);

                    return
                        (lShipNameI < rShipNameI)
                        || ((lShipNameI == rShipNameI) && (l.ShipFileId < r.ShipFileId));
                }
                else
                {
                    assert(l.Metadata->YearBuilt.has_value() != r.Metadata->YearBuilt.has_value());

                    if (l.Metadata->YearBuilt.has_value())
                    {
                        // L has year built, R has not: L on top
                        return true;
                    }
                    else
                    {
                        // L has no year built, R has it: R on top
                        return false;
                    }
                }
            };

            break;
        }
    }

    return [isSortDescending, metadataPredicate=std::move(metadataPredicate)](InfoTile const & l, InfoTile const & r) -> bool
    {
        if (l.Metadata.has_value())
        {
            if (r.Metadata.has_value())
            {
                return metadataPredicate(l, r);
            }
            else
            {
                // L has metadata, R not
                return true; // All metadata-having ones before non-metadata having ones
            }
        }
        else if (r.Metadata.has_value())
        {
            // L has no metadata, R has metadata
            return false; // All metadata-having ones before non-metadata having ones
        }
        else
        {
            // Neither has metadata...
            // ...sort on filename
            return (l.ShipFilepath.filename() < r.ShipFilepath.filename()) != (isSortDescending);
        }
    };
}

ShipPreviewWindow::DirectorySnapshot ShipPreviewWindow::EnumerateShipFiles(std::filesystem::path const & directoryPath)
{
    std::vector<std::tuple<std::filesystem::path, std::filesystem::file_time_type>> files;

    LogMessage("ShipPreviewWindow::EnumerateShipFiles(", directoryPath.string(), "): start...");

    // Be robust to users messing up
    if (std::filesystem::exists(directoryPath)
        && std::filesystem::is_directory(directoryPath))
    {
        auto directoryIterator = std::filesystem::directory_iterator(
            directoryPath,
            std::filesystem::directory_options::skip_permission_denied | std::filesystem::directory_options::follow_directory_symlink);

        for (auto const & entryIt : directoryIterator)
        {
            try
            {
                auto const entryFilepath = entryIt.path();

                if (std::filesystem::is_regular_file(entryFilepath)
                    && ShipDeSerializer::IsAnyShipDefinitionFile(entryFilepath))
                {
                    // Make sure the filename may be converted to the local codepage
                    // (see https://developercommunity.visualstudio.com/content/problem/721120/stdfilesystempathgeneric-string-throws-an-exceptio.html)
                    std::string _ = entryFilepath.filename().string();
                    (void)_;

                    files.emplace_back(
                        entryFilepath,
                        std::filesystem::last_write_time(entryFilepath));
                }
            }
            catch (std::exception const & ex)
            {
                LogMessage("Ignoring a file directory entry due to error: ", ex.what());

                // Ignore this file
            }
        }
    }

    LogMessage("ShipPreviewWindow::EnumerateShipFiles(", directoryPath.string(), "): ...found ", files.size(), " ship files.");

    return DirectorySnapshot(directoryPath, std::move(files));
}

wxBitmap ShipPreviewWindow::MakeBitmap(RgbaImageData const & shipPreviewImage) const
{
    try
    {
        return WxHelpers::MakeBitmap(shipPreviewImage);
    }
    catch (...)
    {
        return WxHelpers::MakeEmptyBitmap();
    }
}

void ShipPreviewWindow::RecalculateGeometry(
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

    // Calculate column width
    mColumnWidth = InfoTileWidth + mExpandedHorizontalMargin;

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
}

size_t ShipPreviewWindow::MapMousePositionToInfoTile(wxPoint const & mousePosition)
{
    wxPoint virtualMouse = CalcUnscrolledPosition(mousePosition);

    assert(mColumnWidth > 0);

    int c = virtualMouse.x / mColumnWidth;
    int r = virtualMouse.y / RowHeight;

    return static_cast<size_t>(c + r * mCols);
}

void ShipPreviewWindow::EnsureInfoTileIsVisible(size_t infoTileIndex)
{
    wxRect const visibleRectVirtual = GetVisibleRectVirtual();

    assert(infoTileIndex < mInfoTiles.size());
    wxRect const infoTileRectVirtual = InfoTileIndexToRectVirtual(infoTileIndex);

    if (!visibleRectVirtual.Contains(infoTileRectVirtual))
    {
        int xUnit, yUnit;
        GetScrollPixelsPerUnit(&xUnit, &yUnit);
        if (yUnit != 0)
        {
            this->Scroll(
                -1,
                infoTileRectVirtual.GetTop() / yUnit);
        }
    }
}

void ShipPreviewWindow::EnsureSelectedShipIsVisible()
{
    if (mSelectedShipFileId.has_value())
    {
        EnsureInfoTileIsVisible(ShipFileIdToInfoTileIndex(*mSelectedShipFileId));
    }
}

wxRect ShipPreviewWindow::GetVisibleRectVirtual() const
{
    wxRect visibleRectVirtual(GetClientSize());
    visibleRectVirtual.Offset(CalcUnscrolledPosition(visibleRectVirtual.GetTopLeft()));

    return visibleRectVirtual;
}

std::tuple<wxString, wxSize> ShipPreviewWindow::CalculateTextSizeWithCurrentFont(
    wxDC & dc,
    std::string const & text)
{
    //
    // Calculate coordinates of text (x is relative to the text bounding rect, y is height),
    // and eventually make ellipsis in text
    //

    wxString wxText(text);

    wxSize textSize = dc.GetTextExtent(wxText);
    while (textSize.GetWidth() > PreviewImageWidth
        && wxText.Len() > 3)
    {
        // Make ellipsis
        wxText.Truncate(wxText.Len() - 4).Append("...");

        // Recalc width now
        textSize = dc.GetTextExtent(wxText);
    }

    return std::make_tuple(wxText, textSize);
}

void ShipPreviewWindow::Render(wxDC & dc)
{
    dc.Clear();

    if (!mInfoTiles.empty())
    {
        // Calculate visible portion in virtual space
        wxRect visibleRectVirtual = GetVisibleRectVirtual();

        // Calculate virtual origin - all virtual coordinates will need this subtracted from them in
        // order to become device coordinates
        wxPoint originVirtual = visibleRectVirtual.GetTopLeft();

        // Calculate left margin for content of info tile
        int const infoTileContentLeftMargin = mExpandedHorizontalMargin / 2 + InfoTileInset;

        // Process all info tiles
        for (size_t i = 0; i < mInfoTiles.size(); ++i)
        {
            wxRect const infoTileRectVirtual = InfoTileIndexToRectVirtual(i);
            auto & infoTile = mInfoTiles[i];

            // Check if this info tile's virtual rect intersects the visible one
            if (visibleRectVirtual.Intersects(infoTileRectVirtual))
            {
                //
                // Bitmap
                //

                dc.DrawBitmap(
                    infoTile.Bitmap,
                    infoTileRectVirtual.GetLeft() + infoTileContentLeftMargin
                        + PreviewImageWidth / 2 - infoTile.Bitmap.GetWidth() / 2
                        - originVirtual.x,
                    infoTileRectVirtual.GetTop() + InfoTileInset
                        + PreviewImageHeight - infoTile.Bitmap.GetHeight()
                        - originVirtual.y,
                    true);

                //
                // Ribbons
                //

                if (infoTile.IsHD)
                {
                    if (infoTile.HasElectricals)
                    {
                        dc.DrawBitmap(
                            mPreviewRibbonBatteryAndHDBitmap,
                            infoTileRectVirtual.GetLeft() + InfoTileInset
                                - originVirtual.x,
                            infoTileRectVirtual.GetTop() + InfoTileInset
                                - originVirtual.y,
                            true);
                    }
                    else
                    {
                        dc.DrawBitmap(
                            mPreviewRibbonHDBitmap,
                            infoTileRectVirtual.GetLeft() + InfoTileInset
                                - originVirtual.x,
                            infoTileRectVirtual.GetTop() + InfoTileInset
                                - originVirtual.y,
                            true);
                    }
                }
                else if (infoTile.HasElectricals)
                {
                    dc.DrawBitmap(
                        mPreviewRibbonBatteryBitmap,
                        infoTileRectVirtual.GetLeft() + InfoTileInset
                            - originVirtual.x,
                        infoTileRectVirtual.GetTop() + InfoTileInset
                            - originVirtual.y,
                        true);
                }

                //
                // Labels
                //

                int const labelCenterX =
                    infoTileRectVirtual.GetLeft() + infoTileContentLeftMargin
                    + PreviewImageWidth / 2
                    - originVirtual.x;

                int nextLabelY =
                    infoTileRectVirtual.GetTop() + InfoTileInset
                    + PreviewImageHeight + PreviewImageBottomMargin
                    - originVirtual.y;

                // Description 1

                dc.SetTextForeground(mDescriptionColor);
                dc.SetFont(mDescriptionFont);

                if (!infoTile.DescriptionLabel1Size)
                {
                    auto const [descr, size] = CalculateTextSizeWithCurrentFont(dc, infoTile.OriginalDescription1);
                    infoTile.DescriptionLabel1 = descr;
                    infoTile.DescriptionLabel1Size = size;
                }

                dc.DrawText(
                    infoTile.DescriptionLabel1,
                    labelCenterX - infoTile.DescriptionLabel1Size->GetWidth() / 2,
                    nextLabelY);

                nextLabelY += infoTile.DescriptionLabel1Size->GetHeight() + DescriptionLabel1BottomMargin;

                // Filename

                dc.SetTextForeground(mFilenameColor);
                dc.SetFont(mFilenameFont);

                if (!infoTile.FilenameLabelSize)
                {
                    auto const [filename, size] = CalculateTextSizeWithCurrentFont(dc, "(" + infoTile.ShipFilepath.filename().string() + ")");
                    infoTile.FilenameLabel = filename;
                    infoTile.FilenameLabelSize = size;
                }

                dc.DrawText(
                    infoTile.FilenameLabel,
                    labelCenterX - infoTile.FilenameLabelSize->GetWidth() / 2,
                    nextLabelY);

                nextLabelY += infoTile.FilenameLabelSize->GetHeight() + FilenameLabelBottomMargin;

                // Description 2

                dc.SetTextForeground(mDescriptionColor);
                dc.SetFont(mDescriptionFont);

                if (!infoTile.DescriptionLabel2Size)
                {
                    auto const [descr, size] = CalculateTextSizeWithCurrentFont(dc, infoTile.OriginalDescription2);
                    infoTile.DescriptionLabel2 = descr;
                    infoTile.DescriptionLabel2Size = size;
                }

                dc.DrawText(
                    infoTile.DescriptionLabel2,
                    labelCenterX - infoTile.DescriptionLabel2Size->GetWidth() / 2,
                    nextLabelY);

                nextLabelY += infoTile.DescriptionLabel2Size->GetHeight() + DescriptionLabel2BottomMargin;

                // Description 3

                if (!infoTile.DescriptionLabel3Size)
                {
                    auto const [descr, size] = CalculateTextSizeWithCurrentFont(dc, infoTile.OriginalDescription3);
                    infoTile.DescriptionLabel3 = descr;
                    infoTile.DescriptionLabel3Size = size;
                }

                dc.DrawText(
                    infoTile.DescriptionLabel3,
                    labelCenterX - infoTile.DescriptionLabel3Size->GetWidth() / 2,
                    nextLabelY);

                //
                // Selection
                //

                if (mInfoTiles[i].ShipFileId == mSelectedShipFileId)
                {
                    dc.SetPen(mSelectionPen);
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                    dc.DrawRectangle(
                        infoTileRectVirtual.GetLeft() + 2 - originVirtual.x,
                        infoTileRectVirtual.GetTop() + 2 - originVirtual.y,
                        infoTileRectVirtual.GetWidth() - 4,
                        infoTileRectVirtual.GetHeight() - 4);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////

void ShipPreviewWindow::ShutdownPreviewThread()
{
    {
        std::lock_guard<std::mutex> lock(mPanelToThreadMessageMutex);

        mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeExitMessage()));
        mPanelToThreadMessageEvent.notify_one();
    }

    // Wait for thread to be done
    mPreviewThread.join();
}

///////////////////////////////////////////////////////////////////////////////////
// Preview Thread
///////////////////////////////////////////////////////////////////////////////////

void ShipPreviewWindow::RunPreviewThread()
{
    LogMessage("PreviewThread::Enter");

    while (true)
    {
        //
        // Check whether there's a message for us
        //
        // Note that we will always see the latest message
        //

        std::unique_ptr<PanelToThreadMessage> message;

        {
            std::unique_lock<std::mutex> lock(mPanelToThreadMessageMutex, std::defer_lock);
            lock.lock();

            mPanelToThreadMessageEvent.wait(
                lock,
                [this] {return !!mPanelToThreadMessage; });

            // Got a message, extract it (we're holding the lock)
            assert(!!mPanelToThreadMessage);
            message = std::move(mPanelToThreadMessage);

            lock.unlock();
        }

        //
        // Process Message
        //

        if (PanelToThreadMessage::MessageType::SetDirectory == message->GetMessageType())
        {
            //
            // Scan directory
            //

            try
            {
                ScanDirectorySnapshot(std::move(message->GetDirectorySnapshot()));
            }
            catch (std::exception const & ex)
            {
                // Send error message
                QueueThreadToPanelMessage(
                    ThreadToPanelMessage::MakeDirScanErrorMessage(
                        ex.what()));
            }
        }
        else if (PanelToThreadMessage::MessageType::InterruptScan == message->GetMessageType())
        {
            //
            // Scan interrupted
            //

            // Notify ack
            std::lock_guard<std::mutex> lock(mThreadToPanelScanInterruptAckMutex);
            mThreadToPanelScanInterruptAck = true;
            mThreadToPanelScanInterruptAckEvent.notify_one();
        }
        else
        {
            assert(PanelToThreadMessage::MessageType::Exit == message->GetMessageType());

            //
            // Exit
            //

            break;
        }
    }

    LogMessage("PreviewThread::Exit");
}

void ShipPreviewWindow::ScanDirectorySnapshot(DirectorySnapshot && directorySnapshot)
{
    LogMessage("PreviewThread::ScanDirectorySnapshot(", directorySnapshot.DirectoryPath.string(), "): processing...");

    auto previewDirectoryManager = ShipPreviewDirectoryManager::Create(directorySnapshot.DirectoryPath);

    //
    // Process all files and create previews
    //

    for (auto fileIt = directorySnapshot.FileEntries.cbegin(); fileIt != directorySnapshot.FileEntries.cend(); ++fileIt)
    {
        // Check whether we have been interrupted
        if (!!mPanelToThreadMessage)
        {
            LogMessage("PreviewThread::ScanDirectorySnapshot(): interrupted, exiting");

            // Commit - with a partial visit
            previewDirectoryManager->Commit(false);

            return;
        }

        try
        {
            // Load preview data
            auto shipPreviewData = ShipDeSerializer::LoadShipPreviewData(fileIt->FilePath);

            // Load preview image
            auto shipPreviewImage = previewDirectoryManager->LoadPreviewImage(shipPreviewData, PreviewImageSize);

            // Notify
            QueueThreadToPanelMessage(
                ThreadToPanelMessage::MakePreviewReadyMessage(
                    fileIt->ShipFileId,
                    std::move(shipPreviewData),
                    std::move(shipPreviewImage)));
        }
        catch (std::exception const & ex)
        {
            LogMessage("PreviewThread::ScanDirectorySnapshot(): encountered error (", std::string(ex.what()), "), notifying...");

            // Notify
            QueueThreadToPanelMessage(
                ThreadToPanelMessage::MakePreviewErrorMessage(
                    fileIt->ShipFileId,
                    "Cannot load preview"));

            LogMessage("PreviewThread::ScanDirectorySnapshot(): ...error notified.");

            // Keep going
        }
    }


    //
    // Notify completion
    //

    QueueThreadToPanelMessage(
        ThreadToPanelMessage::MakePreviewCompletedMessage(
            std::move(directorySnapshot)));

    //
    // Commit - with a full visit
    //

    previewDirectoryManager->Commit(true);

    LogMessage("PreviewThread::ScanDirectorySnapshot(): ...preview completed.");
}

void ShipPreviewWindow::QueueThreadToPanelMessage(std::unique_ptr<ThreadToPanelMessage> message)
{
    // Lock queue
    std::scoped_lock lock(mThreadToPanelMessageQueueMutex);

    // Push message
    mThreadToPanelMessageQueue.push_back(std::move(message));
}