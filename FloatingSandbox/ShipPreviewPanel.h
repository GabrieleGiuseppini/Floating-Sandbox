/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipPreviewControl.h"

#include <Game/ResourceLoader.h>

#include <wx/wx.h>

#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

/*
 * This panel populates itself with previews of all ships found in a directory.
 * The search for ships and extraction of previews is done by a separate thread,
 * so to not interfere with the UI message pump.
 */
class ShipPreviewPanel : public wxScrolled<wxPanel>
{
public:

    ShipPreviewPanel(
        wxWindow* parent,
        ResourceLoader const & resourceLoader);

	virtual ~ShipPreviewPanel();

    void SetDirectory(std::filesystem::path const & directoryPath);

private:

    void OnShow(wxShowEvent & event);
    void OnResized(wxSizeEvent & event);
    void OnShipFileSelected(fsShipFileSelectedEvent & event);

private:

    wxGridSizer * mPreviewsSizer;

    std::shared_ptr<wxBitmap> mWaitBitmap;
    std::shared_ptr<wxBitmap> mErrorBitmap;

private:

    ////////////////////////////////////////////////
    // Preview Thread
    ////////////////////////////////////////////////

    std::thread mPreviewThread;

    void RunPreviewThread();

    //
    // Panel-to-Thread communication
    //

    struct IPanelToThreadMessage
    {
    public:

        enum class MessageType
        {
            SetDirectory,
            Exit
        };

        IPanelToThreadMessage(MessageType messageType)
            : mMessageType(messageType)
        {}

        virtual ~IPanelToThreadMessage() {}

        MessageType GetMessageType() const
        {
            return mMessageType;
        }

    private:

        MessageType const mMessageType;
    };

    // Single message holder - thread only cares about last message
    std::unique_ptr<IPanelToThreadMessage> mPanelToThreadMessage;

    // TODO: cond var & mutex

    //
    // Thread-to-Panel communication
    //

    struct IThreadToPanelMessage
    {
    public:

        enum class MessageType
        {
            Clear,
            SetSize,
            PreviewReady,
            Done,
            Error
        };

        IThreadToPanelMessage(MessageType messageType)
            : mMessageType(messageType)
        {}

        virtual ~IThreadToPanelMessage() {}

        MessageType GetMessageType() const
        {
            return mMessageType;
        }

    private:

        MessageType const mMessageType;
    };

    // Queue of messages for panel, together with its lock and event
    std::deque<std::unique_ptr<IThreadToPanelMessage>> mThreadToPanelMessages;
    std::mutex mThreadToPanelMessagesMutex;
    void FireThreadToPanelMessageReady();
    void OnThreadToPanelMessageReady();

};
