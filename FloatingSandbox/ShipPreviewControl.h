/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ShipPreview.h>

#include <wx/generic/statbmpg.h>
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
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    fsShipFileSelectedEvent(fsShipFileSelectedEvent const & other)
        : wxEvent(other)
        , mShipFilepath(other.mShipFilepath)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    virtual wxEvent *Clone() const override
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
 * This control implements the tile that contains a ship preview.
 */
class ShipPreviewControl : public wxPanel
{
public:

    static constexpr int Width = 200;

    static constexpr int ImageWidth = 200;
    static constexpr int ImageHeight = 150;
    static_assert(ImageWidth <= Width);

public:

    ShipPreviewControl(
        wxWindow * parent,
        std::filesystem::path const & shipFilepath,
        int vMargin,
        RgbaImageData const & waitImage,
        RgbaImageData const & errorImage);

    virtual ~ShipPreviewControl();

    void SetPreviewContent(ShipPreview const & shipPreview);
    void SetPreviewContent(RgbaImageData const & image, std::string const & description);

private:

    void OnMouseSingleClick(wxMouseEvent & event);
    void OnMouseDoubleClick(wxMouseEvent & event);

private:

    void SetImageContent(RgbaImageData const & imageData);

private:

    wxBoxSizer * mVSizer;

    wxPanel * mImagePanel;
    wxGenericStaticBitmap * mImageGenericStaticBitmap;
    wxStaticText * mDescriptionLabel;
    wxStaticText * mFilenameLabel;

private:

    std::filesystem::path const mShipFilepath;
    RgbaImageData const & mWaitImage;
    RgbaImageData const & mErrorImage;
};
