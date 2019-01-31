/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewControl.h"

#include <GameCore/Log.h>

wxDEFINE_EVENT(fsSHIP_FILE_SELECTED, fsShipFileSelectedEvent);

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

    mPreviewStaticBitmap = new wxStaticBitmap(
        this,
        wxID_ANY,
        *mWaitBitmap,
        wxDefaultPosition,
        mWaitBitmap->GetSize(),
        wxBORDER_SIMPLE);

    mPreviewStaticBitmap->SetScaleMode(wxStaticBitmap::Scale_AspectFill);

    mVSizer->Add(mPreviewStaticBitmap, 1, wxEXPAND);

    // TODOTEST
    mPreviewStaticBitmap->Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick1, this);


    //
    // Label
    //

    mPreviewLabel = new wxStaticText(this, wxID_ANY, "Loading...", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

    mVSizer->Add(mPreviewLabel, 0, wxEXPAND);

    Bind(wxEVT_LEFT_DCLICK, &ShipPreviewControl::OnMouseDoubleClick2, this, mPreviewLabel->GetId());


    this->SetSizer(mVSizer);
}

ShipPreviewControl::~ShipPreviewControl()
{
}

void ShipPreviewControl::OnMouseDoubleClick1(wxMouseEvent & /*event*/)
{
    // TODOTEST
    LogMessage("ShipPreviewControl::OnMouseDoubleClick1");

    //
    // Fire our custom event
    //

    fsShipFileSelectedEvent event(
        fsSHIP_FILE_SELECTED,
        this->GetId(),
        mShipFilepath);

    event.SetEventObject(this);

    ProcessWindowEvent(event);
}

void ShipPreviewControl::OnMouseDoubleClick2(wxMouseEvent & /*event*/)
{
    // TODOHERE
    LogMessage("ShipPreviewControl::OnMouseDoubleClick2");
}