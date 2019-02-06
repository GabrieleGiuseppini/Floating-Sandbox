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

#include <wx/wx.h>

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//
// Custom events
//

// A directory has been scanned
class fsDirScannedEvent : public wxEvent
{
public:

    fsDirScannedEvent(
        wxEventType eventType,
        int winid,
        std::vector<std::filesystem::path> const & shipFilepaths)
        : wxEvent(winid, eventType)
        , mShipFilepaths(shipFilepaths)
    {
    }

    fsDirScannedEvent(fsDirScannedEvent const & other)
        : wxEvent(other)
        , mShipFilepaths(other.mShipFilepaths)
    {
    }

    virtual wxEvent *Clone() const override
    {
        return new fsDirScannedEvent(*this);
    }

    std::vector<std::filesystem::path> const & GetShipFilepaths() const
    {
        return mShipFilepaths;
    }

private:
    std::vector<std::filesystem::path> const mShipFilepaths;
};

wxDECLARE_EVENT(fsEVT_DIR_SCANNED, fsDirScannedEvent);

// An error was encountered while scanning a directory
class fsDirScanErrorEvent : public wxEvent
{
public:

    fsDirScanErrorEvent(
        wxEventType eventType,
        int winid,
        std::string const & errorMessage)
        : wxEvent(winid, eventType)
        , mErrorMessage(errorMessage)
    {
    }

    fsDirScanErrorEvent(fsDirScanErrorEvent const & other)
        : wxEvent(other)
        , mErrorMessage(other.mErrorMessage)
    {
    }

    virtual wxEvent *Clone() const override
    {
        return new fsDirScanErrorEvent(*this);
    }

    std::string const & GetErrorMessage() const
    {
        return mErrorMessage;
    }

private:
    std::string const mErrorMessage;
};

wxDECLARE_EVENT(fsEVT_DIR_SCAN_ERROR, fsDirScanErrorEvent);

// A preview is ready
class fsPreviewReadyEvent : public wxEvent
{
public:

    fsPreviewReadyEvent(
        wxEventType eventType,
        int winid,
        size_t shipIndex,
        std::shared_ptr<ShipPreview> shipPreview)
        : wxEvent(winid, eventType)
        , mShipIndex(shipIndex)
        , mShipPreview(std::move(shipPreview))
    {
    }

    fsPreviewReadyEvent(fsPreviewReadyEvent const & other)
        : wxEvent(other)
        , mShipIndex(other.mShipIndex)
        , mShipPreview(std::move(other.mShipPreview))
    {
    }

    virtual wxEvent *Clone() const override
    {
        return new fsPreviewReadyEvent(*this);
    }

    size_t GetShipIndex() const
    {
        return mShipIndex;
    }

    std::shared_ptr<ShipPreview> GetShipPreview()
    {
        return mShipPreview;
    }

private:
    size_t const mShipIndex;
    std::shared_ptr<ShipPreview> mShipPreview;
};

wxDECLARE_EVENT(fsEVT_PREVIEW_READY, fsPreviewReadyEvent);

// An error occurred while preparing a preview
class fsPreviewErrorEvent : public wxEvent
{
public:

    fsPreviewErrorEvent(
        wxEventType eventType,
        int winid,
        size_t shipIndex,
        std::string errorMessage)
        : wxEvent(winid, eventType)
        , mShipIndex(shipIndex)
        , mErrorMessage(errorMessage)
    {
    }

    fsPreviewErrorEvent(fsPreviewErrorEvent const & other)
        : wxEvent(other)
        , mShipIndex(other.mShipIndex)
        , mErrorMessage(other.mErrorMessage)
    {
    }

    virtual wxEvent *Clone() const override
    {
        return new fsPreviewErrorEvent(*this);
    }

    size_t GetShipIndex() const
    {
        return mShipIndex;
    }

    std::string const & GetErrorMessage() const
    {
        return mErrorMessage;
    }

private:

    size_t const mShipIndex;
    std::string const mErrorMessage;
};

wxDECLARE_EVENT(fsEVT_PREVIEW_ERROR, fsPreviewErrorEvent);

// The directory preview was completed
class fsDirPreviewCompleteEvent : public wxEvent
{
public:

    fsDirPreviewCompleteEvent(
        wxEventType eventType,
        int winid)
        : wxEvent(winid, eventType)
    {
    }

    fsDirPreviewCompleteEvent(fsDirPreviewCompleteEvent const & other)
        : wxEvent(other)
    {
    }

    virtual wxEvent *Clone() const override
    {
        return new fsDirPreviewCompleteEvent(*this);
    }
};

wxDECLARE_EVENT(fsEVT_DIR_PREVIEW_COMPLETE, fsDirPreviewCompleteEvent);

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

private:

    void OnResized(wxSizeEvent & event);

private:

    // Thread-to-Panel communication - we use wxWidgets' custom events
    void OnDirScanned(fsDirScannedEvent & event);
    void OnDirScanError(fsDirScanErrorEvent & event);
    void OnPreviewReady(fsPreviewReadyEvent & event);
    void OnPreviewError(fsPreviewErrorEvent & event);
    void OnDirPreviewComplete(fsDirPreviewCompleteEvent & event);

    int CalculateTileColumns();
    void ShutdownPreviewThread();

private:

    int mWidth;
    int mHeight;

    wxPanel * mPreviewPanel;
    wxGridSizer * mPreviewPanelSizer;
    std::vector<ShipPreviewControl *> mPreviewControls;

    RgbaImageData mWaitImage;
    RgbaImageData mErrorImage;

private:

    // The directory currently being previewed
    std::filesystem::path mCurrentDirectory;

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
};
