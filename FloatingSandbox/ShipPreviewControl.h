/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/wx.h>

#include <filesystem>
#include <memory>

/*
 * Event fired when a ship file has been selected.
 */
class fsShipFileSelectedEvent : public wxEvent
{
public:

    fsShipFileSelectedEvent(
        wxEventType eventType,
        int winid,
        std::filesystem::path const & shipFilepath)
        : wxEvent(winid, eventType)
        , mShipFilepath(shipFilepath)
    {
    }

    fsShipFileSelectedEvent(fsShipFileSelectedEvent const & other)
        : wxEvent(other)
        , mShipFilepath(other.mShipFilepath)
    {
    }

    virtual wxEvent *Clone() const
    {
        return new fsShipFileSelectedEvent(*this);
    }

    std::filesystem::path const GetShipFilepath() const
    {
        return mShipFilepath;
    }

private:
    std::filesystem::path const mShipFilepath;
};

wxDECLARE_EVENT(fsSHIP_FILE_SELECTED, fsShipFileSelectedEvent);

/*
 * This control implements the tile that contains a ship preview.
 */
class ShipPreviewControl : public wxPanel
{
public:

    ShipPreviewControl(
        wxWindow * parent,
        std::filesystem::path const & shipFilepath,
        int width,
        int height,
        std::shared_ptr<wxBitmap> waitBitmap,
        std::shared_ptr<wxBitmap> errorBitmap);

    virtual ~ShipPreviewControl();

private:

    void OnMouseDoubleClick1(wxMouseEvent & event);
    void OnMouseDoubleClick2(wxMouseEvent & event);

private:

    wxBoxSizer * mVSizer;
    wxStaticBitmap * mPreviewStaticBitmap;
    wxStaticText * mPreviewLabel;

private:

    std::filesystem::path const mShipFilepath;
    int const mWidth;
    int const mHeight;
    std::shared_ptr<wxBitmap> mWaitBitmap;
    std::shared_ptr<wxBitmap> mErrorBitmap;
};
