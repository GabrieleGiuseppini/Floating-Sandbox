/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-09-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>
#include <Game/ShipPreview.h>

#include <GameCore/ImageData.h>

#include <wx/timer.h>
#include <wx/wx.h>

#include <condition_variable>
#include <deque>
#include <filesystem>
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

    static int constexpr PreviewImageBottomMargin = 9;

    static int constexpr DescriptionLabel1Height = 7;
    static int constexpr DescriptionLabel1BottomMargin = 6;
    static int constexpr DescriptionLabel2Height = 7;
    static int constexpr DescriptionLabel2BottomMargin = 12;
    static int constexpr FilenameLabelHeight = 7;
    static int constexpr FilenameLabelBottomMargin = 0;


    //
    // InfoTile
    //

    static int constexpr InfoTileWidth = InfoTileInset + PreviewImageWidth + InfoTileInset;
    static int constexpr InfoTileHeight =
        InfoTileInset
        + PreviewImageHeight
        + PreviewImageBottomMargin
        + DescriptionLabel1Height
        + DescriptionLabel1BottomMargin
        + DescriptionLabel2Height
        + DescriptionLabel2BottomMargin
        + FilenameLabelHeight
        + FilenameLabelBottomMargin
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
    void OnPollQueueTimer(wxTimerEvent & event);

private:

    void Select(size_t infoTileIndex);

    void Choose(size_t infoTileIndex);

    wxBitmap MakeBitmap(RgbaImageData const & shipPreviewImage) const;

    void RecalculateGeometry(
        wxSize clientSize,
        int nPreviews);

    size_t MapMousePositionToInfoTile(wxPoint const & mousePosition);

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
    wxFont mFilenameFont;

    wxBitmap const mWaitBitmap;
    wxBitmap const mErrorBitmap;
    wxBitmap const mPreviewRibbonBatteryBitmap;
    wxBitmap const mPreviewRibbonHDBitmap;
    wxBitmap const mPreviewRibbonBatteryAndHDBitmap;

private:

    void ShutdownPreviewThread();

    std::unique_ptr<wxTimer> mPollQueueTimer;

    struct InfoTile
    {
        wxBitmap Bitmap;
        bool IsHD;
        bool HasElectricals;
        std::string OriginalDescription1;
        std::string OriginalDescription2;
        std::filesystem::path ShipFilepath;

        wxString Description1;
        std::optional<wxSize> Description1Size;
        wxString Description2;
        std::optional<wxSize> Description2Size;
        wxString Filename;
        std::optional<wxSize> FilenameSize;

        int Col;
        int Row;
        wxRect RectVirtual;

        std::optional<ShipMetadata> Metadata;

        std::vector<std::string> SearchStrings;

        InfoTile(
            wxBitmap bitmap,
            bool isHD,
            bool hasElectricals,
            std::string const & description1,
            std::string const & description2,
            std::filesystem::path const & shipFilepath)
            : Bitmap(bitmap)
            , IsHD(isHD)
            , HasElectricals(hasElectricals)
            , OriginalDescription1(description1)
            , OriginalDescription2(description2)
            , ShipFilepath(shipFilepath)
        {}
    };

    // The info tiles currently populated
    std::vector<InfoTile> mInfoTiles;

    // The currently-selected info tile
    std::optional<size_t> mSelectedInfoTileIndex;

    // When set, indicates that the preview of this directory is completed
    std::optional<std::filesystem::path> mCurrentlyCompletedDirectory;

    ////////////////////////////////////////////////
    // Preview Thread
    ////////////////////////////////////////////////

    std::thread mPreviewThread;

    void RunPreviewThread();
    void ScanDirectory(std::filesystem::path const & directoryPath);

    //
    // Panel-to-Thread communication
    //

    struct PanelToThreadMessage
    {
    public:

        enum class MessageType
        {
            SetDirectory,
            Exit
        };

        static PanelToThreadMessage MakeExitMessage()
        {
            return PanelToThreadMessage(MessageType::Exit, std::filesystem::path());
        }

        static PanelToThreadMessage MakeSetDirectoryMessage(std::filesystem::path const & directoryPath)
        {
            return PanelToThreadMessage(MessageType::SetDirectory, directoryPath);
        }

        PanelToThreadMessage(PanelToThreadMessage && other)
            : mMessageType(other.mMessageType)
            , mDirectoryPath(std::move(other.mDirectoryPath))
        {}

        MessageType GetMessageType() const
        {
            return mMessageType;
        }

        std::filesystem::path const & GetDirectoryPath() const
        {
            return mDirectoryPath;
        }

    private:

        PanelToThreadMessage(
            MessageType messageType,
            std::filesystem::path const & directoryPath)
            : mMessageType(messageType)
            , mDirectoryPath(directoryPath)
        {}

        MessageType const mMessageType;
        std::filesystem::path const mDirectoryPath;
    };

    // Single message holder - thread only cares about last message
    std::unique_ptr<PanelToThreadMessage> mPanelToThreadMessage;

    // Locks for the panel-to-thread message
    std::mutex mPanelToThreadMessageMutex;
    std::unique_lock<std::mutex> mPanelToThreadMessageLock;
    std::condition_variable mPanelToThreadMessageEvent;

    //
    // Thread-to-panel communication
    //

    struct ThreadToPanelMessage
    {
    public:

        enum class MessageType
        {
            DirScanCompleted,
            DirScanError,
            PreviewReady,
            PreviewError,
            PreviewCompleted
        };

        static std::unique_ptr<ThreadToPanelMessage> MakeDirScanCompletedMessage(std::vector<std::filesystem::path> scannedShipFilepaths)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::DirScanCompleted));
            msg->mScannedShipFilepaths = std::move(scannedShipFilepaths);
            return msg;
        }

        static std::unique_ptr<ThreadToPanelMessage> MakeDirScanErrorMessage(std::string errorMessage)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::DirScanError));
            msg->mErrorMessage = std::move(errorMessage);
            return msg;
        }

        static std::unique_ptr<ThreadToPanelMessage> MakePreviewReadyMessage(
            size_t shipIndex,
            ShipPreview && shipPreview,
            RgbaImageData && shipPreviewImage)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewReady));
            msg->mShipIndex = shipIndex;
            msg->mShipPreview.emplace(std::move(shipPreview));
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

        static std::unique_ptr<ThreadToPanelMessage> MakePreviewCompletedMessage(std::filesystem::path scannedDirectoryPath)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewCompleted));
            msg->mScannedDirectoryPath = std::move(scannedDirectoryPath);
            return msg;
        }

        ThreadToPanelMessage(ThreadToPanelMessage && other) = default;

        ThreadToPanelMessage & operator=(ThreadToPanelMessage && other) = default;

        MessageType GetMessageType() const
        {
            return mMessageType;
        }

        std::filesystem::path const & GetScannedDirectoryPath() const
        {
            return mScannedDirectoryPath;
        }

        std::vector<std::filesystem::path> const & GetScannedShipFilepaths() const
        {
            return mScannedShipFilepaths;
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

        ShipPreview const & GetShipPreview()
        {
            return *mShipPreview;
        }

        RgbaImageData const & GetShipPreviewImage()
        {
            return *mShipPreviewImage;
        }

    private:

        ThreadToPanelMessage(MessageType messageType)
            : mMessageType(messageType)
            , mScannedDirectoryPath()
            , mScannedShipFilepaths()
            , mErrorMessage()
            , mShipIndex()
            , mShipPreview()
            , mShipPreviewImage()
        {}

        MessageType mMessageType;

        std::filesystem::path mScannedDirectoryPath;
        std::vector<std::filesystem::path> mScannedShipFilepaths;
        std::string mErrorMessage;
        std::optional<size_t> mShipIndex;
        std::optional<ShipPreview> mShipPreview;
        std::optional<RgbaImageData> mShipPreviewImage;
    };

    void QueueThreadToPanelMessage(std::unique_ptr<ThreadToPanelMessage> message);

    // Queue of messages
    std::deque<std::unique_ptr<ThreadToPanelMessage>> mThreadToPanelMessageQueue;

    // Locks for the thread-to-panel message queue
    std::mutex mThreadToPanelMessageQueueMutex;
};
