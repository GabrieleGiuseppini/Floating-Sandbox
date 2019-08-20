/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipPreviewControl.h"

#include <Game/ResourceLoader.h>
#include <Game/ShipPreview.h>

#include <GameCore/ImageData.h>

#include <wx/timer.h>
#include <wx/wx.h>

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/*
 * This panel populates itself with previews of all ships found in a directory.
 * The search for ships and extraction of previews is done by a separate thread,
 * so to not interfere with the UI message pump.
 */
class ShipPreviewPanel : public wxScrolled<wxPanel>
{
public:

    static constexpr int MinPreviewHGap = 5;
    static constexpr int MinPreviewWidth = ShipPreviewControl::Width + 2 * MinPreviewHGap;
    static constexpr int PreviewVGap = 8;

public:

    ShipPreviewPanel(
        wxWindow* parent,
        ResourceLoader const & resourceLoader);

	virtual ~ShipPreviewPanel();

    void OnOpen();
    void OnClose();

    void SetDirectory(std::filesystem::path const & directoryPath);

    void Search(std::string const & shipName);
    void ChooseSearched();

private:

    void OnResized(wxSizeEvent & event);
    void OnPollQueueTimer(wxTimerEvent & event);
    void OnShipFileSelected(fsShipFileSelectedEvent & event);

private:

    void OnDirScanCompleted(std::vector<std::filesystem::path> const & scannedShipFilepaths);
    // TODO: others

    int CalculateTileColumns();
    void ShutdownPreviewThread();

private:

    int mWidth;
    int mHeight;

    wxPanel * mPreviewPanel;
    wxGridSizer * mPreviewPanelSizer;
    std::vector<ShipPreviewControl *> mPreviewControls;
    std::optional<size_t> mSelectedPreview;

    std::unique_ptr<wxTimer> mPollQueueTimer;

    RgbaImageData mWaitImage;
    RgbaImageData mErrorImage;

private:

    // When set, indicates that the preview of this directory is completed
    std::optional<std::filesystem::path> mCurrentlyCompletedDirectory;

    // Ship name (lcase) to preview index, used when searching for a ship by name
    std::vector<std::string> mShipNameToPreviewIndex;

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
            std::unique_ptr<ShipPreview> shipPreview)
        {
            std::unique_ptr<ThreadToPanelMessage> msg(new ThreadToPanelMessage(MessageType::PreviewReady));
            msg->mShipIndex = shipIndex;
            msg->mShipPreview = std::move(shipPreview);
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

    private:

        ThreadToPanelMessage(MessageType messageType)
            : mMessageType(messageType)
            , mScannedDirectoryPath()
            , mScannedShipFilepaths()
            , mErrorMessage()
            , mShipIndex()
            , mShipPreview()
        {}

        MessageType const mMessageType;

        std::filesystem::path mScannedDirectoryPath;
        std::vector<std::filesystem::path> mScannedShipFilepaths;
        std::string mErrorMessage;
        std::optional<size_t> mShipIndex;
        std::unique_ptr<ShipPreview> mShipPreview;
    };

    void QueueThreadToPanelMessage(std::unique_ptr<ThreadToPanelMessage> message);

    // Queue of messages
    std::deque<std::unique_ptr<ThreadToPanelMessage>> mThreadToPanelMessageQueue;

    // Locks for the thread-to-panel message queue
    std::mutex mThreadToPanelMessageQueueMutex;
};
