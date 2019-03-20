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

    std::filesystem::path const GetShipFilepath() const
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
 * This control implements the tile that contains a ship preview.
 */
class ShipPreviewControl : public wxPanel
{
public:

    static constexpr int ImageWidth = 200;
    static constexpr int ImageHeight = 150;
    static constexpr int BorderSize = 1;

    static constexpr int Width = ImageWidth + 2 * BorderSize;

    static_assert(ImageWidth <= Width);

public:

    ShipPreviewControl(
        wxWindow * parent,
        size_t shipIndex,
        std::filesystem::path const & shipFilepath,
        int vMargin,
        RgbaImageData const & waitImage,
        RgbaImageData const & errorImage);

    virtual ~ShipPreviewControl();

    void SetSelected(bool isSelected);

    void SetPreviewContent(ShipPreview const & shipPreview);

    void SetPreviewContent(
        RgbaImageData const & image,
        std::string const & description1,
        std::string const & description2);

private:

    void OnMouseSingleClick(wxMouseEvent & event);
    void OnMouseDoubleClick(wxMouseEvent & event);

private:

    void SetImageContent(RgbaImageData const & imageData);

private:

    wxBoxSizer * mVSizer;

    wxPanel * mBackgroundPanel;
    wxPanel * mImagePanel;
    wxGenericStaticBitmap * mImageGenericStaticBitmap;
    wxStaticText * mDescriptionLabel1;
    wxStaticText * mDescriptionLabel2;
    wxStaticText * mFilenameLabel;

private:

    size_t const mShipIndex;
    std::filesystem::path const mShipFilepath;
    RgbaImageData const & mWaitImage;
    RgbaImageData const & mErrorImage;

    // State
    std::optional<ShipMetadata> mShipMetadata;
};
