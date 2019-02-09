/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewControl.h"

#include <GameCore/Log.h>

#include <wx/rawbmp.h>

wxDEFINE_EVENT(fsEVT_SHIP_FILE_SELECTED, fsShipFileSelectedEvent);
wxDEFINE_EVENT(fsEVT_SHIP_FILE_CHOSEN, fsShipFileChosenEvent);

ShipPreviewControl::ShipPreviewControl(
    wxWindow * parent,
    size_t shipIndex,
    std::filesystem::path const & shipFilepath,
    int vMargin,
    RgbaImageData const & waitImage,
    RgbaImageData const & errorImage)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition)
    , mImageGenericStaticBitmap(nullptr)
    , mShipIndex(shipIndex)
    , mShipFilepath(shipFilepath)
    , mWaitImage(waitImage)
    , mErrorImage(errorImage)
{
    SetBackgroundColour(wxColour("WHITE"));


    //
    // Background panel
    //

    mBackgroundPanel = new wxPanel(this);

    mBackgroundPanel->SetBackgroundColour(wxColour("WHITE"));

    mBackgroundPanel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mBackgroundPanel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);



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

    mImagePanel->SetMinSize(wxSize(ImageWidth, ImageHeight));
    mImagePanel->SetMaxSize(wxSize(ImageWidth, ImageHeight));

    mImagePanel->Bind(wxEVT_LEFT_DOWN, &ShipPreviewControl::OnMouseSingleClick, this);
    mImagePanel->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick, this);

    // Create sizer that we'll use to size image
    auto imageSizer = new wxBoxSizer(wxVERTICAL);
    mImagePanel->SetSizer(imageSizer);

    // Set initial content to "Wait" image
    SetImageContent(mWaitImage);

    mVSizer->Add(mImagePanel, 1, wxALIGN_CENTER_HORIZONTAL);

    mVSizer->AddSpacer(4);



    //
    // Description Labels
    //

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


    //
    // Bottom margin
    //

    mVSizer->AddSpacer(vMargin);


    //
    // Finalize content
    //

    mBackgroundPanel->SetSizer(mVSizer);


    //
    // Finalize this panel
    //

    // Wrap the background panel with border
    wxSizer * backgroundSizer = new wxBoxSizer(wxVERTICAL);
    backgroundSizer->Add(mBackgroundPanel, 0, wxALL, BorderSize);

    this->SetSizer(backgroundSizer);
}

ShipPreviewControl::~ShipPreviewControl()
{
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
        + " m/"
        + std::to_string(feet)
        + " ft";

    if (!!shipPreview.Metadata.Author)
        descriptionLabelText2 += " - by " + *(shipPreview.Metadata.Author);

    //
    // Set content
    //

    SetPreviewContent(
        shipPreview.PreviewImage,
        descriptionLabelText1,
        descriptionLabelText2);
}

void ShipPreviewControl::SetPreviewContent(
    RgbaImageData const & image,
    std::string const & description1,
    std::string const & description2)
{
    SetImageContent(image);
    mImagePanel->Refresh();

    mDescriptionLabel1->SetLabel(description1);
    mDescriptionLabel2->SetLabel(description2);

    // Rearrange
    mVSizer->Layout();
}

void ShipPreviewControl::OnMouseSingleClick(wxMouseEvent & /*event*/)
{
    //
    // Fire our custom event
    //

    auto event = fsShipFileSelectedEvent(
        fsEVT_SHIP_FILE_SELECTED,
        this->GetId(),
        mShipIndex,
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

void ShipPreviewControl::SetImageContent(RgbaImageData const & imageData)
{
    //
    // Create bitmap with content
    //

    wxBitmap bitmap(imageData.Size.Width, imageData.Size.Height, 32);

    wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
    if (!pixelData)
    {
        throw std::exception("Cannot get bitmap pixel data");
    }

    assert(pixelData.GetWidth() == imageData.Size.Width);
    assert(pixelData.GetHeight() == imageData.Size.Height);

    rgbaColor const * readIt = imageData.Data.get();
    auto writeIt = pixelData.GetPixels();
    writeIt.OffsetY(pixelData, imageData.Size.Height - 1);
    for (int y = 0; y < imageData.Size.Height; ++y)
    {
        // Save current iterator
        auto rowStart = writeIt;

        for (int x = 0; x < imageData.Size.Width; ++x, ++readIt, ++writeIt)
        {
            writeIt.Red() = readIt->r;
            writeIt.Green() = readIt->g;
            writeIt.Blue() = readIt->b;
            writeIt.Alpha() = readIt->a;
        }

        // Move write iterator to next row
        writeIt = rowStart;
        writeIt.OffsetY(pixelData, -1);
    }


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