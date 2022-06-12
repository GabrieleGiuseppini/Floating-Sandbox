/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-09-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>
#include <Game/ShipPreviewData.h>

#include <GameCore/ImageData.h>

#include <wx/timer.h>
#include <wx/wx.h>

#include <cassert>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

/*
 * Event fired when a ship file has been selected.
 */
class fsShipFileSelectedEvent : public wxEvent
{
public:

    fsShipFileSelectedEvent(
        wxEventType eventType,
        int winid,
        size_t shipIndex,
        std::optional<ShipMetadata> const & shipMetadata,
        std::filesystem::path const & shipFilepath)
        : wxEvent(winid, eventType)
        , mShipIndex(shipIndex)
        , mShipMetadata(shipMetadata)
        , mShipFilepath(shipFilepath)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    fsShipFileSelectedEvent(fsShipFileSelectedEvent const & other)
        : wxEvent(other)
        , mShipIndex(other.mShipIndex)
        , mShipMetadata(other.mShipMetadata)
        , mShipFilepath(other.mShipFilepath)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    virtual wxEvent *Clone() const override
    {
        return new fsShipFileSelectedEvent(*this);
    }

    size_t GetShipIndex() const
    {
        return mShipIndex;
    }

    std::optional<ShipMetadata> const & GetShipMetadata() const
    {
        return mShipMetadata;
    }

    std::filesystem::path const & GetShipFilepath() const
    {
        return mShipFilepath;
    }

private:

    size_t const mShipIndex;
    std::optional<ShipMetadata> const mShipMetadata;
    std::filesystem::path const mShipFilepath;
};

wxDECLARE_EVENT(fsEVT_SHIP_FILE_SELECTED, fsShipFileSelectedEvent);

/*
 * Event fired when a ship file has been chosen.
 */
class fsShipFileChosenEvent : public wxEvent
{
public:

    fsShipFileChosenEvent(
        wxEventType eventType,
        int winid,
        std::filesystem::path const & shipFilepath)
        : wxEvent(winid, eventType)
        , mShipFilepath(shipFilepath)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    fsShipFileChosenEvent(fsShipFileChosenEvent const & other)
        : wxEvent(other)
        , mShipFilepath(other.mShipFilepath)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    virtual wxEvent *Clone() const override
    {
        return new fsShipFileChosenEvent(*this);
    }

    std::filesystem::path const GetShipFilepath() const
    {
        return mShipFilepath;
    }

private:
    std::filesystem::path const mShipFilepath;
};

wxDECLARE_EVENT(fsEVT_SHIP_FILE_CHOSEN, fsShipFileChosenEvent);

/*
 * This window populates itself with previews of all ships found in a directory.
 * The search for ships and extraction of previews is done by a separate thread,
 * so to not interfere with the UI message pump.
 */
class ShipPreviewWindow : public wxScrolled<wxWindow>
{
public:

    //
    // InfoTile components
    //

    static int constexpr InfoTileInset = 4; // For selection

    static int constexpr PreviewImageWidth = 200;
    static int constexpr PreviewImageHeight = 150;
    static ImageSize constexpr PreviewImageSize = ImageSize(PreviewImageWidth, PreviewImageHeight);

    static int constexpr PreviewImageBottomMargin = 7;

    static int constexpr DescriptionLabel1BottomMargin = 0;
    static int constexpr FilenameLabelBottomMargin = 0;
    static int constexpr DescriptionLabel2BottomMargin = 0;
    static int constexpr DescriptionLabel3BottomMargin = 12;

    //
    // InfoTile
    //

    static int constexpr InfoTileWidth = InfoTileInset + PreviewImageWidth + InfoTileInset;
    static int constexpr InfoTileHeight =
        InfoTileInset
        + PreviewImageHeight
        + PreviewImageBottomMargin
        + 7 // Description 1 height
        + DescriptionLabel1BottomMargin
        + 5 // Filename height
        + FilenameLabelBottomMargin
        + 7 // Description 2 height
        + DescriptionLabel2BottomMargin
        + 7 // Description 3 height
        + DescriptionLabel3BottomMargin
        + InfoTileInset;

    static int constexpr HorizontalMarginMin = 4;
    static int constexpr VerticalMargin = 8;

    //
    // Grid
    //

    static int constexpr ColumnWidthMin = InfoTileWidth + HorizontalMarginMin;
    static int constexpr RowHeight = InfoTileHeight + VerticalMargin;

    // Minimum width to ensure one info tile == one column width
    static int constexpr PanelWidthMin = ColumnWidthMin;

    static int constexpr CalculateMinWidthForColumns(int nCols)
    {
        return
            HorizontalMarginMin / 2
            + nCols * InfoTileWidth
            + (nCols - 1) * HorizontalMarginMin
            + HorizontalMarginMin / 2;
    }

public:

    ShipPreviewWindow(
        wxWindow* parent,
        ResourceLocator const & resourceLocator);

    virtual ~ShipPreviewWindow();

    void OnOpen();
    void OnClose();

    void SetDirectory(std::filesystem::path const & directoryPath);

    bool Search(std::string const & shipName);
    void ChooseSelectedIfAny();

private:

    void OnPaint(wxPaintEvent & event);
    void OnResized(wxSizeEvent & event);
    void OnMouseSingleClick(wxMouseEvent & event);
    void OnMouseDoubleClick(wxMouseEvent & event);
    void OnKeyDown(wxKeyEvent & event);
    void OnPollQueueTimer(wxTimerEvent & event);

private:

    struct DirectorySnapshot
    {
        std::filesystem::path DirectoryPath;
        std::map<std::filesystem::path, std::filesystem::file_time_type> Files;

        DirectorySnapshot(
            std::filesystem::path const & directoryPath,
            std::map<std::filesystem::path, std::filesystem::file_time_type> && files)
            : DirectoryPath(directoryPath)
            , Files(std::move(files))
        {}
    };

    struct InfoTile
    {
        wxBitmap Bitmap;
        bool IsHD;
        bool HasElectricals;
        std::string OriginalDescription1;
        std::string OriginalDescription2;
        std::string OriginalDescription3;
        std::filesystem::path ShipFilepath;

        wxString Description1;
        std::optional<wxSize> Description1Size;
        wxString Description2;
        std::optional<wxSize> Description2Size;
        wxString Description3;
        std::optional<wxSize> Description3Size;
        wxString Filename;
        std::optional<wxSize> FilenameSize;

        int Col;
        int Row;
        wxRect RectVirtual;

        std::optional<ShipMetadata> Metadata;

        std::vector<std::string> SearchStrings;

        InfoTile(
            wxBitmap bitmap,
            std::filesystem::path const & shipFilepath)
            : Bitmap(bitmap)
            , IsHD(false)
            , HasElectricals(false)
            , OriginalDescription1()
            , OriginalDescription2()
            , OriginalDescription3()
            , ShipFilepath(shipFilepath)
        {}
    };

private:

    void Select(size_t infoTileIndex);

    void Choose(size_t infoTileIndex);

    void ResetInfoTiles(DirectorySnapshot const & directorySnapshot);

    static std::map<std::filesystem::path, std::filesystem::file_time_type> EnumerateShipFiles(std::filesystem::path const & directoryPath);

    wxBitmap MakeBitmap(RgbaImageData const & shipPreviewImage) const;

    void RecalculateGeometry(
        wxSize clientSize,
        int nPreviews);

    size_t MapMousePositionToInfoTile(wxPoint const & mousePosition);

    void EnsureTileIsVisible(size_t infoTileIndex);

    wxRect GetVisibleRectVirtual() const;

    std::tuple<wxString, wxSize> CalculateTextSizeWithCurrentFont(
        wxDC & dc,
        std::string const & text);

    void Render(wxDC & dc);

    wxSize mClientSize;
    int mVirtualHeight;
    int mCols;
    int mRows;
    int mColumnWidth;
    int mExpandedHorizontalMargin;

    wxPen mSelectionPen;    
    wxFont mDescriptionFont;
    wxColor mDescriptionColor;
    wxFont mFilenameFont;
    wxColor mFilenameColor;

    wxBitmap const mWaitBitmap;
    wxBitmap const mErrorBitmap;
    wxBitmap const mPreviewRibbonBatteryBitmap;
    wxBitmap const mPreviewRibbonHDBitmap;
    wxBitmap const mPreviewRibbonBatteryAndHDBitmap;

private:

    void ShutdownPreviewThread();

    std::unique_ptr<wxTimer> mPollQueueTimer;

    // The info tiles currently populated
    std::vector<InfoTile> mInfoTiles;

    // The currently-selected info tile
    std::optional<size_t> mSelectedInfoTileIndex;

    // When set, indicates that the preview of this directory is completed
    std::optional<DirectorySnapshot> mCurrentlyCompletedDirectorySnapshot;

    ////////////////////////////////////////////////
    // Preview Thread
    ////////////////////////////////////////////////

    std::thread mPreviewThread;

    void RunPreviewThread();
    void ScanDirectorySnapshot(DirectorySnapshot && directorySnapshot);

    //
    // Panel-to-Thread communication
    //

    struct PanelToThreadMessage
    {
    public:

        enum class MessageType
        {
            SetDirectory,
            InterruptScan,
            Exit
        };

        static PanelToThreadMessage MakeSetDirectoryMessage(DirectorySnapshot && directorySnapshot)
        {
            return PanelToThreadMessage(MessageType::SetDirectory, std::move(directorySnapshot));
        }

        static PanelToThreadMessage MakeInterruptScanMessage()
        {
            return PanelToThreadMessage(MessageType::InterruptScan, std::nullopt);
        }

        static PanelToThreadMessage MakeExitMessage()
        {
            return PanelToThreadMessage(MessageType::Exit, std::nullopt);
        }

        PanelToThreadMessage(PanelToThreadMessage && other) noexcept
            : mMessageType(other.mMessageType)
            , mDirectorySnapshot(std::move(other.mDirectorySnapshot))
        {}

        MessageType GetMessageType() const
        {
            return mMessageType;
        }

        DirectorySnapshot const & GetDirectorySnapshot() const
        {
            assert(mDirectorySnapshot.has_value());
            return *mDirectorySnapshot;
        }

        DirectorySnapshot && GetDirectorySnapshot()
        {
            return std::move(*mDirectorySnapshot);
        }

    private:

        PanelToThreadMessage(
            MessageType messageType,
            std::optional<DirectorySnapshot> && directorySnapshot)
            : mMessageType(messageType)
            , mDirectorySnapshot(std::move(directorySnapshot))
        {}

        MessageType const mMessageType;
        std::optional<DirectorySnapshot> mDirectorySnapshot;
    };

    // Single message holder - thread only cares about last message
    std::unique_ptr<PanelToThreadMessage> mPanelToThreadMessage;

    // Locks for the panel-to-thread message
    std::mutex mPanelToThreadMessageMutex;
    std::condition_variable mPanelToThreadMessageEvent;

    //
    // Thread-to-panel communication
    //

    struct ThreadToPanelMessage
    {
    public:

        enum class MessageType
        {
            DirScanError,
            PreviewReady,
            PreviewError,
            PreviewCompleted
        };

        static std::unique_ptr<ThreadToPanelMessage> MakeDirScanErrorMessage(std::string errorMessage)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::DirScanError));
            msg->mErrorMessage = std::move(errorMessage);
            return msg;
        }

        static std::unique_ptr<ThreadToPanelMessage> MakePreviewReadyMessage(
            size_t shipIndex,
            ShipPreviewData && shipPreviewData,
            RgbaImageData && shipPreviewImage)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewReady));
            msg->mShipIndex = shipIndex;
            msg->mShipPreviewData.emplace(std::move(shipPreviewData));
            msg->mShipPreviewImage.emplace(std::move(shipPreviewImage));
            return msg;
        }

        static std::unique_ptr<ThreadToPanelMessage> MakePreviewErrorMessage(
            size_t shipIndex,
            std::string errorMessage)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewError));
            msg->mShipIndex = shipIndex;
            msg->mErrorMessage = std::move(errorMessage);
            return msg;
        }

        static std::unique_ptr<ThreadToPanelMessage> MakePreviewCompletedMessage(DirectorySnapshot && directorySnapshot)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewCompleted));
            msg->mDirectorySnapshot.emplace(std::move(directorySnapshot));
            return msg;
        }

        ThreadToPanelMessage(ThreadToPanelMessage && other) = default;

        ThreadToPanelMessage & operator=(ThreadToPanelMessage && other) = default;

        MessageType GetMessageType() const
        {
            return mMessageType;
        }

        DirectorySnapshot const & GetDirectorySnapshot() const
        {
            assert(mDirectorySnapshot.has_value());
            return *mDirectorySnapshot;
        }

        std::string const & GetErrorMessage() const
        {
            return mErrorMessage;
        }

        size_t GetShipIndex() const
        {
            assert(!!mShipIndex);
            return *mShipIndex;
        }

        ShipPreviewData const & GetShipPreviewData()
        {
            return *mShipPreviewData;
        }

        RgbaImageData const & GetShipPreviewImage()
        {
            return *mShipPreviewImage;
        }

    private:

        ThreadToPanelMessage(MessageType messageType)
            : mMessageType(messageType)
            , mDirectorySnapshot()
            , mErrorMessage()
            , mShipIndex()
            , mShipPreviewData()
            , mShipPreviewImage()
        {}

        MessageType mMessageType;

        std::optional<DirectorySnapshot> mDirectorySnapshot;
        std::string mErrorMessage;
        std::optional<size_t> mShipIndex;
        std::optional<ShipPreviewData> mShipPreviewData;
        std::optional<RgbaImageData> mShipPreviewImage;
    };

    void QueueThreadToPanelMessage(std::unique_ptr<ThreadToPanelMessage> message);

    // Queue of messages
    std::deque<std::unique_ptr<ThreadToPanelMessage>> mThreadToPanelMessageQueue;

    // Locks for the thread-to-panel message queue
    std::mutex mThreadToPanelMessageQueueMutex;

    // Scan interrupted ack
    bool mThreadToPanelScanInterruptAck;
    std::mutex mThreadToPanelScanInterruptAckMutex;
    std::condition_variable mThreadToPanelScanInterruptAckEvent;
};
