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
    int vMargin,
    std::shared_ptr<wxBitmap> waitBitmap,
    std::shared_ptr<wxBitmap> errorBitmap)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(width, height),
        0)
    , mShipFilepath(shipFilepath)
    , mWidth(width)
    , mHeight(height)
    , mWaitBitmap(std::move(waitBitmap))
    , mErrorBitmap(std::move(errorBitmap))
{
    SetBackgroundColour(wxColour("WHITE"));


    mVSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Preview image
    //

    mPreviewBitmap = new wxStaticBitmap(
        this,
        wxID_ANY,
        *mWaitBitmap,
        wxDefaultPosition,
        mWaitBitmap->GetSize());

    mPreviewBitmap->SetScaleMode(wxStaticBitmap::Scale_AspectFill);

    mPreviewBitmap->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mPreviewBitmap->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mPreviewBitmap, 1, wxEXPAND);



    //
    // Preview Label
    //

    mPreviewLabel = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

    mPreviewLabel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mPreviewLabel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mPreviewLabel, 0, wxEXPAND);



    //
    // Filename Label
    //

    mFilenameLabel = new wxStaticText(this, wxID_ANY, shipFilepath.filename().string(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

    mFilenameLabel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mFilenameLabel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mFilenameLabel, 0, wxEXPAND);

    mVSizer->AddSpacer(vMargin);



    this->SetSizer(mVSizer);
}

ShipPreviewControl::~ShipPreviewControl()
{
}

void ShipPreviewControl::SetPreviewContent(ShipPreview const & shipPreview)
{
    //
    // Set preview image
    //

    // TODOTEST
    ////wxBitmap bmp(
    ////    reinterpret_cast<char const *>(shipPreview.PreviewImage.Data.get()),
    ////    shipPreview.PreviewImage.Size.Width,
    ////    shipPreview.PreviewImage.Size.Height,
    ////    4);

    ////mPreviewBitmap->SetBitmap(bmp);


    //
    // Set preview label
    //

    std::string previewLabelText = shipPreview.Metadata.ShipName;

    if (!!shipPreview.Metadata.YearBuilt)
        previewLabelText += " (" + *(shipPreview.Metadata.YearBuilt) + ")";

    if (!!shipPreview.Metadata.Author)
        previewLabelText += " by " + *(shipPreview.Metadata.Author);

    mPreviewLabel->SetLabel(previewLabelText);


    // Rearrange
    mVSizer->Layout();
}

void ShipPreviewControl::OnMouseSingleClick(wxMouseEvent & /*event*/)
{
    //
    // Set border
    //

    this->SetWindowStyle(wxBORDER_SIMPLE);
    // TODO: if this works, need "Select/Deselect" invoked by panel


    //
    // Fire our custom event
    //

    auto event = fsShipFileSelectedEvent(
        fsEVT_SHIP_FILE_SELECTED,
        this->GetId(),
        mShipFilepath);

    ProcessWindowEvent(event);
}

void ShipPreviewControl::OnMouseDoubleClick(wxMouseEvent & /*event*/)
{
    //
    // Fire our custom event
    //

    auto event = fsShipFileChosenEvent(
        fsEVT_SHIP_FILE_CHOSEN,
        this->GetId(),
        mShipFilepath);

    ProcessWindowEvent(event);
}