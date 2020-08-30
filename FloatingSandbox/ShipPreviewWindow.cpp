/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-09-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewWindow.h"

#include "WxHelpers.h"

#include <Game/ImageFileTools.h>
#include <Game/ShipPreviewDirectoryManager.h>

#include <GameCore/GameException.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

wxDEFINE_EVENT(fsEVT_SHIP_FILE_SELECTED, fsShipFileSelectedEvent);
wxDEFINE_EVENT(fsEVT_SHIP_FILE_CHOSEN, fsShipFileChosenEvent);

ShipPreviewWindow::ShipPreviewWindow(
    wxWindow* parent,
    ResourceLocator const & resourceLocator)
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
    , mWaitBitmap(WxHelpers::MakeBitmap(ImageFileTools::LoadImageRgba(resourceLocator.GetBitmapFilePath("ship_preview_wait"))))
    , mErrorBitmap(WxHelpers::MakeBitmap(ImageFileTools::LoadImageRgba(resourceLocator.GetBitmapFilePath("ship_preview_error"))))
    , mPreviewRibbonBatteryBitmap(WxHelpers::MakeBitmap(ImageFileTools::LoadImageRgba(resourceLocator.GetBitmapFilePath("ship_preview_ribbon_battery"))))
    , mPreviewRibbonHDBitmap(WxHelpers::MakeBitmap(ImageFileTools::LoadImageRgba(resourceLocator.GetBitmapFilePath("ship_preview_ribbon_hd"))))
    , mPreviewRibbonBatteryAndHDBitmap(WxHelpers::MakeBitmap(ImageFileTools::LoadImageRgba(resourceLocator.GetBitmapFilePath("ship_preview_ribbon_battery_and_hd"))))
    //
    , mPollQueueTimer()
    , mInfoTiles()
    , mSelectedInfoTileIndex()
    , mCurrentlyCompletedDirectory()
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
    mFilenameFont = wxFont(wxFontInfo(7).Italic());

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
    assert(!mSelectedInfoTileIndex);

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

    mSelectedInfoTileIndex.reset();
}

void ShipPreviewWindow::SetDirectory(std::filesystem::path const & directoryPath)
{
    // Check if different than current
    if (directoryPath != mCurrentlyCompletedDirectory)
    {
        //
        // Stop thread's scan (if thread it's running)
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
        }

        //
        // Change directory
        //

        mCurrentlyCompletedDirectory.reset();

        // Clear state
        mInfoTiles.clear();
        mSelectedInfoTileIndex.reset();

        // Start thread's scan (if thread is not running now, it'll pick it up when it starts)
        {
            std::lock_guard<std::mutex> lock(mPanelToThreadMessageMutex);

            mPanelToThreadMessage.reset(new PanelToThreadMessage(PanelToThreadMessage::MakeSetDirectoryMessage(directoryPath)));
            mPanelToThreadMessageEvent.notify_one();
        }
    }
}

bool ShipPreviewWindow::Search(std::string const & shipName)
{
    assert(!shipName.empty());

    std::string const shipNameLCase = Utils::ToLower(shipName);

    //
    // Find first ship that contains the requested name as a substring,
    // doing a circular search from the currently-selected ship
    //

    std::optional<size_t> foundShipIndex;
    size_t sOffset = mSelectedInfoTileIndex ? (*mSelectedInfoTileIndex + 1) : 0;
    for (size_t i = 0; i < mInfoTiles.size(); ++i)
    {
        size_t s = (sOffset + i) % mInfoTiles.size();

        if (std::any_of(
            mInfoTiles[s].SearchStrings.cbegin(),
            mInfoTiles[s].SearchStrings.cend(),
            [&shipNameLCase](auto const & str)
            {
                return str.find(shipNameLCase) != std::string::npos;
            }))
        {
            foundShipIndex = s;
            break;
        }
    }

    if (!!foundShipIndex)
    {
        //
        // Scroll to the item if it's not fully visible
        //

        EnsureTileIsVisible(*foundShipIndex);

        //
        // Select item
        //

        Select(*foundShipIndex);
    }

    return foundShipIndex.has_value();
}

void ShipPreviewWindow::ChooseSelectedIfAny()
{
    if (!!mSelectedInfoTileIndex)
    {
        Choose(*mSelectedInfoTileIndex);
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
        Select(selectedInfoTileIndex);
    }

    // Allow focus move
    event.Skip();
}

void ShipPreviewWindow::OnMouseDoubleClick(wxMouseEvent & event)
{
    auto selectedInfoTileIndex = MapMousePositionToInfoTile(event.GetPosition());
    if (selectedInfoTileIndex < mInfoTiles.size())
    {
        Choose(selectedInfoTileIndex);
    }
}

void ShipPreviewWindow::OnKeyDown(wxKeyEvent & event)
{
    if (!mSelectedInfoTileIndex.has_value())
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
        Choose(*mSelectedInfoTileIndex);
        return;
    }
    else
    {
        event.Skip();
        return;
    }

    if (deltaElement != 0)
    {
        int const newIndex = static_cast<int>(*mSelectedInfoTileIndex) + deltaElement;
        if (newIndex >= 0 && newIndex < static_cast<int>(mInfoTiles.size()))
        {
            Select(static_cast<size_t>(newIndex));

            // Move into view if needed
            EnsureTileIsVisible(newIndex);
        }
    }
}

void ShipPreviewWindow::OnPollQueueTimer(wxTimerEvent & /*event*/)
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

                for (size_t s = 0; s < message->GetScannedShipFilepaths().size(); ++s)
                {
                    mInfoTiles.emplace_back(
                        mWaitBitmap,
                        false,
                        false,
                        "",
                        "",
                        message->GetScannedShipFilepaths()[s]);

                    // Add ship filename to search map
                    mInfoTiles[s].SearchStrings.push_back(
                        Utils::ToLower(
                            message->GetScannedShipFilepaths()[s].filename().string()));
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
                //
                // Populate info tile
                //

                assert(message->GetShipIndex() < mInfoTiles.size());

                auto & infoTile = mInfoTiles[message->GetShipIndex()];

                infoTile.Bitmap = MakeBitmap(message->GetShipPreviewImage());
                infoTile.IsHD = message->GetShipPreview().IsHD;
                infoTile.HasElectricals = message->GetShipPreview().HasElectricals;

                std::string descriptionLabelText1 = message->GetShipPreview().Metadata.ShipName;
                if (!!message->GetShipPreview().Metadata.YearBuilt)
                    descriptionLabelText1 += " (" + *(message->GetShipPreview().Metadata.YearBuilt) + ")";
                infoTile.OriginalDescription1 = std::move(descriptionLabelText1);
                infoTile.Description1Size.reset();

                int metres = message->GetShipPreview().OriginalSize.Width;
                int feet = static_cast<int>(round(3.28f * metres));
                std::string descriptionLabelText2 =
                    std::to_string(metres)
                    + "m/"
                    + std::to_string(feet)
                    + "ft";
                if (!!message->GetShipPreview().Metadata.Author)
                    descriptionLabelText2 += " - by " + *(message->GetShipPreview().Metadata.Author);
                infoTile.OriginalDescription2 = std::move(descriptionLabelText2);
                infoTile.Description2Size.reset();

                infoTile.Metadata.emplace(message->GetShipPreview().Metadata);

                // Add ship name to search map
                infoTile.SearchStrings.push_back(
                    Utils::ToLower(
                        message->GetShipPreview().Metadata.ShipName));

                // Add author to search map
                if (!!message->GetShipPreview().Metadata.Author)
                {
                    infoTile.SearchStrings.push_back(
                        Utils::ToLower(
                            *(message->GetShipPreview().Metadata.Author)));
                }

                // Add ship year to search map
                if (!!message->GetShipPreview().Metadata.YearBuilt)
                {
                    infoTile.SearchStrings.push_back(
                        Utils::ToLower(
                            *(message->GetShipPreview().Metadata.YearBuilt)));
                }

                // Remember we need to refresh now
                doRefresh = true;

                break;
            }

            case ThreadToPanelMessage::MessageType::PreviewError:
            {
                // Set error image
                assert(message->GetShipIndex() < mInfoTiles.size());
                mInfoTiles[message->GetShipIndex()].Bitmap = mErrorBitmap;
                mInfoTiles[message->GetShipIndex()].OriginalDescription1 = message->GetErrorMessage();
                mInfoTiles[message->GetShipIndex()].Description1Size.reset();

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

/////////////////////////////////////////////////////////////////////////////////

void ShipPreviewWindow::Select(size_t infoTileIndex)
{
    assert(infoTileIndex < mInfoTiles.size());

    bool isDirty = (mSelectedInfoTileIndex != infoTileIndex);

    mSelectedInfoTileIndex = infoTileIndex;

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
            infoTileIndex,
            mInfoTiles[infoTileIndex].Metadata,
            mInfoTiles[infoTileIndex].ShipFilepath);

        ProcessWindowEvent(event);
    }
}

void ShipPreviewWindow::Choose(size_t infoTileIndex)
{
    assert(infoTileIndex < mInfoTiles.size());

    //
    // Fire chosen event
    //

    auto event = fsShipFileChosenEvent(
        fsEVT_SHIP_FILE_CHOSEN,
        this->GetId(),
        mInfoTiles[infoTileIndex].ShipFilepath);

    ProcessWindowEvent(event);
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

    // Update all info tiles's rectangles
    for (size_t i = 0; i < mInfoTiles.size(); ++i)
    {
        mInfoTiles[i].Description1Size.reset();
        mInfoTiles[i].Description2Size.reset();
        mInfoTiles[i].FilenameSize.reset();

        mInfoTiles[i].Col = static_cast<int>(i % mCols);
        mInfoTiles[i].Row = static_cast<int>(i / mCols);

        int x = mInfoTiles[i].Col * mColumnWidth;
        int y = mInfoTiles[i].Row * RowHeight;
        mInfoTiles[i].RectVirtual = wxRect(x, y, mColumnWidth, RowHeight);
    }
}

size_t ShipPreviewWindow::MapMousePositionToInfoTile(wxPoint const & mousePosition)
{
    wxPoint virtualMouse = CalcUnscrolledPosition(mousePosition);

    assert(mColumnWidth > 0);

    int c = virtualMouse.x / mColumnWidth;
    int r = virtualMouse.y / RowHeight;

    return static_cast<size_t>(c + r * mCols);
}

void ShipPreviewWindow::EnsureTileIsVisible(size_t infoTileIndex)
{
    assert(infoTileIndex < mInfoTiles.size());

    wxRect visibleRectVirtual = GetVisibleRectVirtual();
    if (!visibleRectVirtual.Contains(mInfoTiles[infoTileIndex].RectVirtual))
    {
        int xUnit, yUnit;
        GetScrollPixelsPerUnit(&xUnit, &yUnit);
        if (yUnit != 0)
        {
            this->Scroll(
                -1,
                mInfoTiles[infoTileIndex].RectVirtual.GetTop() / yUnit);
        }
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
            auto & infoTile = mInfoTiles[i];

            // Check if this info tile's virtual rect intersects the visible one
            if (visibleRectVirtual.Intersects(infoTile.RectVirtual))
            {
                //
                // Bitmap
                //

                dc.DrawBitmap(
                    infoTile.Bitmap,
                    infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                        + PreviewImageWidth / 2 - infoTile.Bitmap.GetWidth() / 2
                        - originVirtual.x,
                    infoTile.RectVirtual.GetTop() + InfoTileInset
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
                            infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                            - originVirtual.x,
                            infoTile.RectVirtual.GetTop() + InfoTileInset
                            - originVirtual.y,
                            true);
                    }
                    else
                    {
                        dc.DrawBitmap(
                            mPreviewRibbonHDBitmap,
                            infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                            - originVirtual.x,
                            infoTile.RectVirtual.GetTop() + InfoTileInset
                            - originVirtual.y,
                            true);
                    }
                }
                else if (infoTile.HasElectricals)
                {
                    dc.DrawBitmap(
                        mPreviewRibbonBatteryBitmap,
                        infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                        - originVirtual.x,
                        infoTile.RectVirtual.GetTop() + InfoTileInset
                        - originVirtual.y,
                        true);
                }

                //
                // Description 1
                //

                dc.SetFont(mDescriptionFont);

                if (!infoTile.Description1Size)
                {
                    auto [descr, size] = CalculateTextSizeWithCurrentFont(dc, infoTile.OriginalDescription1);
                    infoTile.Description1 = descr;
                    infoTile.Description1Size = size;
                }

                dc.DrawText(
                    infoTile.Description1,
                    infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                        + PreviewImageWidth / 2 - infoTile.Description1Size->GetWidth() / 2
                        - originVirtual.x,
                    infoTile.RectVirtual.GetTop() + InfoTileInset
                        + PreviewImageHeight + PreviewImageBottomMargin
                        + DescriptionLabel1Height - infoTile.Description1Size->GetHeight()
                        - originVirtual.y);

                //
                // Description 2
                //

                if (!infoTile.Description2Size)
                {
                    auto[descr, size] = CalculateTextSizeWithCurrentFont(dc, infoTile.OriginalDescription2);
                    infoTile.Description2 = descr;
                    infoTile.Description2Size = size;
                }

                dc.DrawText(
                    infoTile.Description2,
                    infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                        + PreviewImageWidth / 2 - infoTile.Description2Size->GetWidth() / 2
                        - originVirtual.x,
                    infoTile.RectVirtual.GetTop() + InfoTileInset
                        + PreviewImageHeight + PreviewImageBottomMargin
                            + DescriptionLabel1Height + DescriptionLabel1BottomMargin
                        + DescriptionLabel2Height - infoTile.Description2Size->GetHeight()
                        - originVirtual.y);

                //
                // Filename
                //

                dc.SetFont(mFilenameFont);

                if (!infoTile.FilenameSize)
                {
                    auto[descr, size] = CalculateTextSizeWithCurrentFont(dc, infoTile.ShipFilepath.filename().string());
                    infoTile.Filename = descr;
                    infoTile.FilenameSize = size;
                }

                dc.DrawText(
                    infoTile.Filename,
                    infoTile.RectVirtual.GetLeft() + infoTileContentLeftMargin
                        + PreviewImageWidth / 2 - infoTile.FilenameSize->GetWidth() / 2
                        - originVirtual.x,
                    infoTile.RectVirtual.GetTop() + InfoTileInset
                        + PreviewImageHeight + PreviewImageBottomMargin
                            + DescriptionLabel1Height + DescriptionLabel1BottomMargin
                            + DescriptionLabel2Height + DescriptionLabel2BottomMargin
                        + FilenameLabelHeight - infoTile.FilenameSize->GetHeight()
                            - originVirtual.y);

                //
                // Selection
                //

                if (i == mSelectedInfoTileIndex)
                {
                    dc.SetPen(mSelectionPen);
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                    dc.DrawRectangle(
                        infoTile.RectVirtual.GetLeft() + 2 - originVirtual.x,
                        infoTile.RectVirtual.GetTop() + 2 - originVirtual.y,
                        infoTile.RectVirtual.GetWidth() - 4,
                        infoTile.RectVirtual.GetHeight() - 4);
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

void ShipPreviewWindow::ScanDirectory(std::filesystem::path const & directoryPath)
{
    LogMessage("PreviewThread::ScanDirectory(", directoryPath.string(), "): processing...");

    auto previewDirectoryManager = ShipPreviewDirectoryManager::Create(directoryPath);

    //
    // Get list of ship files and fire event
    //

    std::vector<std::filesystem::path> shipFilePaths = previewDirectoryManager->EnumerateShipFilePaths();

    QueueThreadToPanelMessage(
        ThreadToPanelMessage::MakeDirScanCompletedMessage(
            shipFilePaths));

    //
    // Process all files and create previews
    //

    for (size_t iShip = 0; iShip < shipFilePaths.size(); ++iShip)
    {
        // Check whether we have been interrupted
        if (!!mPanelToThreadMessage)
        {
            LogMessage("PreviewThread::ScanDirectory(): interrupted, exiting");

            // Commit - with a partial visit
            previewDirectoryManager->Commit(false);

            return;
        }

        try
        {
            // Load preview
            auto shipPreview = ShipPreview::Load(shipFilePaths[iShip]);

            // Load preview image
            auto shipPreviewImage = previewDirectoryManager->LoadPreviewImage(shipPreview, PreviewImageSize);

            // Notify
            QueueThreadToPanelMessage(
                ThreadToPanelMessage::MakePreviewReadyMessage(
                    iShip,
                    std::move(shipPreview),
                    std::move(shipPreviewImage)));

            // Removed with ship preview database
            //// Take it easy a bit
            ////std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (std::exception const & ex)
        {
            LogMessage("PreviewThread::ScanDirectory(): encountered error (", std::string(ex.what()), "), notifying...");

            // Notify
            QueueThreadToPanelMessage(
                ThreadToPanelMessage::MakePreviewErrorMessage(
                    iShip,
                    ex.what()));

            LogMessage("PreviewThread::ScanDirectory(): ...error notified.");

            // Keep going
        }
    }


    //
    // Notify completion
    //

    QueueThreadToPanelMessage(
        ThreadToPanelMessage::MakePreviewCompletedMessage(
            directoryPath));

    //
    // Commit - with a full visit
    //

    previewDirectoryManager->Commit(true);

    LogMessage("PreviewThread::ScanDirectory(): ...preview completed.");
}

void ShipPreviewWindow::QueueThreadToPanelMessage(std::unique_ptr<ThreadToPanelMessage> message)
{
    // Lock queue
    std::scoped_lock lock(mThreadToPanelMessageQueueMutex);

    // Push message
    mThreadToPanelMessageQueue.push_back(std::move(message));
}