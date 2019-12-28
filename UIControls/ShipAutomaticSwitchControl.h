/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>
#include <GameCore/Log.h>

#include <wx/bitmap.h>
#include <wx/stattext.h>
#include <wx/wx.h>

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class ShipAutomaticSwitchControl : public wxPanel
{
public:

    ShipAutomaticSwitchControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        std::function<void(SwitchState)> onSwitchToggled,
        SwitchState currentState)
        : wxPanel(
            parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxBORDER_NONE)
        , mOnEnabledImage(onEnabledImage)
        , mOffEnabledImage(offEnabledImage)
        , mOnDisabledImage(onDisabledImage)
        , mOffDisabledImage(onDisabledImage)
        , mOnSwitchToggled(std::move(onSwitchToggled))
        , mImageBitmap(nullptr)
        , mCurrentState(currentState)
        , mIsEnabled(true)
    {
        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        mImageBitmap = new wxStaticBitmap(this, wxID_ANY, mOnEnabledImage, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&ShipAutomaticSwitchControl::OnMouseClick, this);
        vSizer->Add(mImageBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);

        wxStaticText * labelStaticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);
        vSizer->Add(labelStaticText, 1, wxALIGN_CENTRE_HORIZONTAL);

        this->SetSizerAndFit(vSizer);
    }

    void SetState(SwitchState state)
    {
        mCurrentState = state;

        SetImageForCurrentState();
    }

    void ToggleState()
    {
        SetState(mCurrentState == SwitchState::On ? SwitchState::Off : SwitchState::On);
    }

    void SetEnabled(bool isEnabled)
    {
        mIsEnabled = isEnabled;

        SetImageForCurrentState();
    }

private:

    void OnMouseClick(wxMouseEvent & event)
    {
        // TODO
        LogMessage("Click!");
    }

    void SetImageForCurrentState()
    {
        // TODO
    }

private:

    wxBitmap const & mOnEnabledImage;
    wxBitmap const & mOffEnabledImage;
    wxBitmap const & mOnDisabledImage;
    wxBitmap const & mOffDisabledImage;

    std::function<void(SwitchState)> const mOnSwitchToggled;

    wxStaticBitmap * mImageBitmap;

private:

    SwitchState mCurrentState;
    bool mIsEnabled;
};
