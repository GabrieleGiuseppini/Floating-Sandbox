/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewControl.h"

#include "WxHelpers.h"

#include <GameCore/Log.h>

#include <wx/rawbmp.h>
#include <wx/wupdlock.h>

wxDEFINE_EVENT(fsEVT_SHIP_FILE_SELECTED, fsShipFileSelectedEvent);
wxDEFINE_EVENT(fsEVT_SHIP_FILE_CHOSEN, fsShipFileChosenEvent);

ShipPreviewControl::ShipPreviewControl(
    wxWindow * parent,
    size_t shipIndex,
    std::filesystem::path const & shipFilepath,
    int vMargin,
    wxBitmap const & waitBitmap,
    wxBitmap const & errorBitmap)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition)
    , mImageGenericStaticBitmap(nullptr)
    , mShipIndex(shipIndex)
    , mShipFilepath(shipFilepath)
    , mWaitBitmap(waitBitmap)
    , mErrorBitmap(errorBitmap)
    , mShipMetadata()
{
    LogMessage("TODOHERE:B-B-A");

    SetBackgroundColour(wxColour("WHITE"));


    //
    // Background panel
    //

    LogMessage("TODOHERE:B-B-B");

    mBackgroundPanel = new wxPanel(this);

    mBackgroundPanel->SetBackgroundColour(wxColour("WHITE"));

    mBackgroundPanel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mBackgroundPanel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    LogMessage("TODOHERE:B-B-D");

    //
    // Content
    //

    mVSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Image panel
    //

    mImagePanel = new wxPanel(
        mBackgroundPanel,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(ImageWidth, ImageHeight));

    LogMessage("TODOHERE:B-B-G");

    mImagePanel->SetMinSize(wxSize(ImageWidth, ImageHeight));
    mImagePanel->SetMaxSize(wxSize(ImageWidth, ImageHeight));

    LogMessage("TODOHERE:B-B-I");

    mImagePanel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mImagePanel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    LogMessage("TODOHERE:B-B-J");

    // Create sizer that we'll use to size image
    auto imageSizer = new wxBoxSizer(wxVERTICAL);
    mImagePanel->SetSizer(imageSizer);

    LogMessage("TODOHERE:B-B-L");

    // Set initial content to "Wait" bitmap
    SetImageContent(mWaitBitmap);

    LogMessage("TODOHERE:B-B-M");

    mVSizer->Add(mImagePanel, 1, wxALIGN_CENTER_HORIZONTAL);

    LogMessage("TODOHERE:B-B-N");

    mVSizer->AddSpacer(4);



    //
    // Description Labels
    //

    LogMessage("TODOHERE:B-B-O");

    mDescriptionLabel1 = new wxStaticText(
        mBackgroundPanel,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxSize(Width, -1),
        wxST_NO_AUTORESIZE | wxALIGN_CENTER_HORIZONTAL | wxST_ELLIPSIZE_END);
    mDescriptionLabel1->SetFont(wxFont(wxFontInfo(7)));
    mDescriptionLabel1->SetMaxSize(wxSize(Width, -1));

    mDescriptionLabel1->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mDescriptionLabel1->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mDescriptionLabel1, 0, wxEXPAND);

    LogMessage("TODOHERE:B-B-R");

    mDescriptionLabel2 = new wxStaticText(
        mBackgroundPanel,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxSize(Width, -1),
        wxST_NO_AUTORESIZE | wxALIGN_CENTER_HORIZONTAL | wxST_ELLIPSIZE_END);
    mDescriptionLabel2->SetFont(wxFont(wxFontInfo(7)));
    mDescriptionLabel2->SetMaxSize(wxSize(Width, -1));

    mDescriptionLabel2->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mDescriptionLabel2->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mDescriptionLabel2, 0, wxEXPAND);

    mVSizer->AddSpacer(4);


    LogMessage("TODOHERE:B-B-T");

    //
    // Filename Label
    //

    mFilenameLabel = new wxStaticText(
        mBackgroundPanel,
        wxID_ANY,
        shipFilepath.filename().string(),
        wxDefaultPosition,
        wxSize(Width, -1),
        wxST_NO_AUTORESIZE | wxALIGN_CENTER_HORIZONTAL | wxST_ELLIPSIZE_END);
    mFilenameLabel->SetFont(wxFont(wxFontInfo(7).Italic()));
    mFilenameLabel->SetMaxSize(wxSize(Width, -1));

    mFilenameLabel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mFilenameLabel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mVSizer->Add(mFilenameLabel, 0, wxEXPAND);

    LogMessage("TODOHERE:B-B-U");

    //
    // Bottom margin
    //

    mVSizer->AddSpacer(vMargin);


    //
    // Finalize content
    //

    LogMessage("TODOHERE:B-B-W");

    mBackgroundPanel->SetSizer(mVSizer);


    //
    // Finalize this panel
    //

    LogMessage("TODOHERE:B-B-X");

    // Wrap the background panel with border
    wxSizer * backgroundSizer = new wxBoxSizer(wxVERTICAL);
    backgroundSizer->Add(mBackgroundPanel, 0, wxALL, BorderSize);

    LogMessage("TODOHERE:B-B-Y");

    this->SetSizer(backgroundSizer);

    LogMessage("TODOHERE:B-B-Z");
}

ShipPreviewControl::~ShipPreviewControl()
{
}

void ShipPreviewControl::Select()
{
    //
    // Fire our custom event
    //

    auto event = fsShipFileSelectedEvent(
        fsEVT_SHIP_FILE_SELECTED,
        this->GetId(),
        mShipIndex,
        mShipMetadata,
        mShipFilepath);

    ProcessWindowEvent(event);
}

void ShipPreviewControl::Choose()
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

void ShipPreviewControl::SetSelected(bool isSelected)
{
    // Set border
    this->SetBackgroundColour(isSelected ? wxColour("BLACK") : wxColour("WHITE"));
    this->Refresh();
}

void ShipPreviewControl::SetPreviewContent(ShipPreview const & shipPreview)
{
    //
    // Create bitmap with content
    //

    wxBitmap bitmap;
    try
    {
        bitmap = WxHelpers::MakeBitmap(shipPreview.PreviewImage);
    }
    catch (...)
    {
        //
        // Error, use 1x1 white bitmap
        //

        bitmap.Create(1, 1, 32);

        wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
        if (!pixelData)
        {
            throw std::runtime_error("Cannot get bitmap pixel data");
        }

        auto writeIt = pixelData.GetPixels();
        writeIt.Red() = 0xff;
        writeIt.Green() = 0xff;
        writeIt.Blue() = 0xff;
        writeIt.Alpha() = 0x00;
    }


    //
    // Store ship metadata
    //

    mShipMetadata.emplace(shipPreview.Metadata);


    //
    // Create description text 1
    //

    std::string descriptionLabelText1 = shipPreview.Metadata.ShipName;

    if (!!shipPreview.Metadata.YearBuilt)
        descriptionLabelText1 += " (" + *(shipPreview.Metadata.YearBuilt) + ")";


    //
    // Create description text 2
    //

    int metres = shipPreview.OriginalSize.Width;
    int feet = static_cast<int>(round(3.28f * metres));

    std::string descriptionLabelText2 =
        std::to_string(metres)
        + "m/"
        + std::to_string(feet)
        + "ft";

    if (!!shipPreview.Metadata.Author)
        descriptionLabelText2 += " - by " + *(shipPreview.Metadata.Author);


    //
    // Set content
    //

    SetPreviewContent(
        bitmap,
        descriptionLabelText1,
        descriptionLabelText2);
}

void ShipPreviewControl::SetPreviewContent(
    wxBitmap const & bitmap,
    std::string const & description1,
    std::string const & description2)
{
    // Freeze updates until we're done
    wxWindowUpdateLocker locker(this);

    // Set image
    SetImageContent(bitmap);

    // Set labels
    mDescriptionLabel1->SetLabel(description1);
    mDescriptionLabel2->SetLabel(description2);

    // Rearrange
    mVSizer->Layout();
}

void ShipPreviewControl::OnMouseSingleClick(wxMouseEvent & /*event*/)
{
    this->Select();
}

void ShipPreviewControl::OnMouseDoubleClick(wxMouseEvent & /*event*/)
{
    this->Choose();
}

void ShipPreviewControl::SetImageContent(wxBitmap const & bitmap)
{
    //
    // Create new static bitmap
    //

    // Destroy previous static bitmap
    if (nullptr != mImageGenericStaticBitmap)
        mImageGenericStaticBitmap->Destroy();

    mImageGenericStaticBitmap = new wxGenericStaticBitmap(
        mImagePanel,
        wxID_ANY,
        bitmap,
        wxDefaultPosition,
        wxSize(ImageWidth, ImageHeight));

    mImageGenericStaticBitmap->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mImageGenericStaticBitmap->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    mImagePanel->GetSizer()->AddStretchSpacer(1);
    mImagePanel->GetSizer()->Add(mImageGenericStaticBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);
    mImagePanel->GetSizer()->Layout();
}