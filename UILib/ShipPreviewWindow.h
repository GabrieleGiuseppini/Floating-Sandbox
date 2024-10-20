/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-09-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>
#include <Game/ShipPreviewData.h>

#include <GameCore/ImageData.h>
#include <GameCore/PortableTimepoint.h>
#include <GameCore/StrongTypeDef.h>

#include <wx/timer.h>
#include <wx/wx.h>

#include <cassert>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

/*
 * This window populates itself with previews of all ships found in a directory.
 * The search for ships and extraction of previews is done by a separate thread,
 * so to not interfere with the UI message pump.
 */
class ShipPreviewWindow : public wxScrolled<wxWindow>
{
public:

    using ShipFileId_t = StrongTypeDef<size_t, struct _tagShipFileId>;

    //
    // Event fired when a ship file has been selected.
    //

    class fsShipFileSelectedEvent : public wxEvent
    {
    public:

        fsShipFileSelectedEvent(
            wxEventType eventType,
            int winid,
            std::optional<ShipMetadata> const & shipMetadata,
            std::filesystem::path const & shipFilepath)
            : wxEvent(winid, eventType)
            , mShipMetadata(shipMetadata)
            , mShipFilepath(shipFilepath)
        {
            m_propagationLevel = wxEVENT_PROPAGATE_MAX;
        }

        fsShipFileSelectedEvent(fsShipFileSelectedEvent const & other)
            : wxEvent(other)
            , mShipMetadata(other.mShipMetadata)
            , mShipFilepath(other.mShipFilepath)
        {
            m_propagationLevel = wxEVENT_PROPAGATE_MAX;
        }

        virtual wxEvent * Clone() const override
        {
            return new fsShipFileSelectedEvent(*this);
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

        std::optional<ShipMetadata> const mShipMetadata;
        std::filesystem::path const mShipFilepath;
    };

    //
    // Event fired when a ship file has been chosen.
    //

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

        virtual wxEvent * Clone() const override
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

    //
    // Sort method
    //

    enum class SortMethod
    {
        ByName = 0,
        ByLastModified = 1,
        ByYearBuilt = 2,
        ByFeatures = 3
    };

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

    SortMethod GetCurrentSortMethod() const
    {
        return mSortMethod;
    }

    void SetSortMethod(SortMethod sortMethod);

    bool GetCurrentIsSortDescending() const
    {
        return mIsSortDescending;
    }

    void SetIsSortDescending(bool isSortDescending);

    std::optional<std::filesystem::path> ChooseShipRandomly(std::optional<std::filesystem::path> currentShip) const;

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

        struct FileEntry
        {
            std::filesystem::path FilePath;
            std::filesystem::file_time_type FileLastModified;
            ShipFileId_t ShipFileId;

            FileEntry(
                std::filesystem::path const & filePath,
                std::filesystem::file_time_type const & fileLastModified,
                ShipFileId_t shipFileId)
                : FilePath(filePath)
                , FileLastModified(fileLastModified)
                , ShipFileId(shipFileId)
            {}
        };

        std::vector<FileEntry> FileEntries;

        DirectorySnapshot(
            std::filesystem::path const & directoryPath,
            std::vector<std::tuple<std::filesystem::path, std::filesystem::file_time_type>> && files)
            : DirectoryPath(directoryPath)
        {
            for (auto const & file : files)
            {
                FileEntries.emplace_back(
                    std::get<0>(file),
                    std::get<1>(file),
                    ShipFileId_t(FileEntries.size())); // Here we assign the ID once and for all
            }

            // Sort just to have a deterministic order - needed for comparisons between DirectorySnapshot's
            std::sort(
                FileEntries.begin(),
                FileEntries.end(),
                [](auto const & l, auto const & r) -> bool
                {
                    return l.FilePath < r.FilePath;
                });
        }

        bool IsEquivalentTo(DirectorySnapshot const & other) const
        {
            if (DirectoryPath != other.DirectoryPath)
                return false;

            if (FileEntries.size() != other.FileEntries.size())
                return false;

            for (size_t f = 0; f < FileEntries.size(); ++f)
            {
                if (FileEntries[f].FilePath != other.FileEntries[f].FilePath
                    || FileEntries[f].FileLastModified != other.FileEntries[f].FileLastModified)
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct InfoTile
    {
        ShipFileId_t ShipFileId;
        std::filesystem::path ShipFilepath;

        wxBitmap Bitmap;

        bool IsHD;
        bool HasElectricals;
        int FeatureScore;
        PortableTimepoint LastWriteTime;
        std::string OriginalDescription1;
        std::string OriginalDescription2;
        std::string OriginalDescription3;

        wxString DescriptionLabel1;
        std::optional<wxSize> DescriptionLabel1Size;
        wxString DescriptionLabel2;
        std::optional<wxSize> DescriptionLabel2Size;
        wxString DescriptionLabel3;
        std::optional<wxSize> DescriptionLabel3Size;
        wxString FilenameLabel;
        std::optional<wxSize> FilenameLabelSize;

        std::optional<ShipMetadata> Metadata;

        std::vector<std::string> SearchStrings;

        InfoTile(
            ShipFileId_t shipFileId,
            std::filesystem::path const & shipFilepath,
            wxBitmap bitmap)
            : ShipFileId(shipFileId)
            , ShipFilepath(shipFilepath)
            , Bitmap(bitmap)
            , IsHD(false)
            , HasElectricals(false)
            , FeatureScore(0)
            , LastWriteTime(0)
            , OriginalDescription1()
            , OriginalDescription2()
            , OriginalDescription3()
        {}
    };

private:

    void SelectInfoTile(size_t infoTileIndex);

    void ChooseInfoTile(size_t infoTileIndex);

    void ResetInfoTiles(DirectorySnapshot const & directorySnapshot);

    void SortInfoTiles();

    void ResortInfoTile(size_t infoTileIndex);

    size_t ShipFileIdToInfoTileIndex(ShipFileId_t shipFileId) const;

    wxRect InfoTileIndexToRectVirtual(size_t infoTileIndex) const;

    void RefreshSortPredicate();

    static std::function<bool(InfoTile const &, InfoTile const &)> MakeSortPredicate(SortMethod sortMethod, bool isSortDescending);

    DirectorySnapshot EnumerateShipFiles(std::filesystem::path const & directoryPath);

    wxBitmap MakeBitmap(RgbaImageData const & shipPreviewImage) const;

    void RecalculateGeometry(
        wxSize clientSize,
        int nPreviews);

    size_t MapMousePositionToInfoTile(wxPoint const & mousePosition);

    void EnsureInfoTileIsVisible(size_t infoTileIndex);

    void EnsureSelectedShipIsVisible();

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

    // The info tiles currently populated; always sorted by the current sort method
    std::vector<InfoTile> mInfoTiles;

    // The currently-selected ship file ID (hence info tile index)
    std::optional<ShipFileId_t> mSelectedShipFileId;

    // The current sorting of the info tiles
    SortMethod mSortMethod;
    bool mIsSortDescending;
    std::function<bool(InfoTile const &, InfoTile const &)> mSortPredicate;

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
            ShipFileId_t shipFileId,
            ShipPreviewData && shipPreviewData,
            RgbaImageData && shipPreviewImage)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewReady));
            msg->mShipFileId = shipFileId;
            msg->mShipPreviewData.emplace(std::move(shipPreviewData));
            msg->mShipPreviewImage.emplace(std::move(shipPreviewImage));
            return msg;
        }

        static std::unique_ptr<ThreadToPanelMessage> MakePreviewErrorMessage(
            ShipFileId_t shipFileId,
            std::string errorMessage)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewError));
            msg->mShipFileId = shipFileId;
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

        ShipFileId_t GetShipFileId() const
        {
            assert(mShipFileId.has_value());
            return *mShipFileId;
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
            , mShipFileId()
            , mShipPreviewData()
            , mShipPreviewImage()
        {}

        MessageType mMessageType;

        std::optional<DirectorySnapshot> mDirectorySnapshot;
        std::string mErrorMessage;
        std::optional<ShipFileId_t> mShipFileId;
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

wxDECLARE_EVENT(fsEVT_SHIP_FILE_SELECTED, ShipPreviewWindow::fsShipFileSelectedEvent);
wxDECLARE_EVENT(fsEVT_SHIP_FILE_CHOSEN, ShipPreviewWindow::fsShipFileChosenEvent);
