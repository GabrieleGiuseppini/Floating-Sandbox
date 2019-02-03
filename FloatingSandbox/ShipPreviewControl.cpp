/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewControl.h"

#include <GameCore/Log.h>

wxDEFINE_EVENT(fsEVT_SHIP_FILE_SELECTED, fsShipFileSelectedEvent);
wxDEFINE_EVENT(fsEVT_SHIP_FILE_CHOSEN, fsShipFileChosenEvent);

ShipPreviewControl::ShipPreviewControl(
    wxWindow * parent,
    std::filesystem::path const & shipFilepath,
    int width,
    int height,
    std::shared_ptr<wxBitmap> waitBitmap,
    std::shared_ptr<wxBitmap> errorBitmap)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(width, height),
        wxBORDER_SIMPLE)
    , mShipFilepath(shipFilepath)
    , mWidth(width)
    , mHeight(height)
    , mWaitBitmap(std::move(waitBitmap))
    , mErrorBitmap(std::move(errorBitmap))
{
    SetBackgroundColour(wxColour("WHITE"));

    wxFont font(wxFontInfo(wxSize(8, 8)).Family(wxFONTFAMILY_TELETYPE));
    SetFont(font);


    mVSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Preview image
    //

    mPreviewBitmap = new wxStaticBitmap(
        this,
        wxID_ANY,
        *mWaitBitmap,
        wxDefaultPosition,
        mWaitBitmap->GetSize(),
        wxBORDER_SIMPLE);

    mPreviewBitmap->SetScaleMode(wxStaticBitmap::Scale_AspectFill);

    mPreviewBitmap->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mPreviewBitmap->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mPreviewBitmap, 1, wxEXPAND);



    //
    // Preview Label
    //

    mPreviewLabel = new wxStaticText(this, wxID_ANY, "Loading...", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

    mPreviewLabel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mPreviewLabel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mPreviewLabel, 0, wxEXPAND);



    //
    // Filename Label
    //

    mFilenameLabel = new wxStaticText(this, wxID_ANY, shipFilepath.stem().string(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

    mFilenameLabel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mFilenameLabel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mFilenameLabel, 0, wxEXPAND);




    this->SetSizer(mVSizer);
}

ShipPreviewControl::~ShipPreviewControl()
{
}

void ShipPreviewControl::SetPreviewContent(
    ImageData previewImageData,
    ShipMetadata const & shipMetadata)
{
    //
    // Set preview image
    //

    wxBitmap * bmp = new wxBitmap(
        reinterpret_cast<char const *>(previewImageData.Data.get()),
        previewImageData.Size.Width,
        previewImageData.Size.Height,
        4);

    mPreviewBitmap->SetBitmap(*bmp);


    //
    // Set preview label
    //

    std::string previewLabelTest = shipMetadata.ShipName;
    if (!!shipMetadata.Author)
        previewLabelTest += " by " + *(shipMetadata.Author);

    mPreviewLabel->SetLabel(previewLabelTest);
}

void ShipPreviewControl::OnMouseSingleClick(wxMouseEvent & /*event*/)
{
    //
    // Fire our custom event
    //

    fsShipFileSelectedEvent event(
        fsEVT_SHIP_FILE_SELECTED,
        this->GetId(),
        mShipFilepath);

    event.SetEventObject(this);

    ProcessWindowEvent(event);
}

void ShipPreviewControl::OnMouseDoubleClick(wxMouseEvent & /*event*/)
{
    //
    // Fire our custom event
    //

    fsShipFileChosenEvent event(
        fsEVT_SHIP_FILE_CHOSEN,
        this->GetId(),
        mShipFilepath);

    event.SetEventObject(this);

    ProcessWindowEvent(event);
}