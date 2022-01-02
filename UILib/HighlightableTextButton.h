/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-11-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/button.h>
#include <wx/settings.h>

class HighlightableTextButton : public wxButton
{
public:

    HighlightableTextButton(
        wxWindow * parent,
        wxString const & label)
        : HighlightableTextButton(
            parent,
            label,
            wxColor(0, 120, 215),
            *wxWHITE,
            wxColor(96, 171, 230),
            *wxBLACK)
    {}

    HighlightableTextButton(
        wxWindow * parent,
        wxString const & label,
        wxColor backgroundUnfocusedHighlightColor,
        wxColor foregroundUnfocusedHighlightColor,
        wxColor backgroundFocusedHighlightColor,
        wxColor foregroundFocusedHighlightColor)
        : wxButton(
            parent,
            wxID_ANY,
            label,
            wxDefaultPosition,
            wxDefaultSize)
        , mBackgroundUnfocusedHighlightColor(backgroundUnfocusedHighlightColor)
        , mForegroundUnfocusedHighlightColor(foregroundUnfocusedHighlightColor)
        , mBackgroundFocusedHighlightColor(backgroundFocusedHighlightColor)
        , mForegroundFocusedHighlightColor(foregroundFocusedHighlightColor)
        , mIsHighlighted(false)
    {
        Bind(wxEVT_ENTER_WINDOW,
            [this](wxMouseEvent & /*event*/)
            {
                SetColors(true);
            });

        Bind(wxEVT_LEAVE_WINDOW,
            [this](wxMouseEvent & /*event*/)
            {
                SetColors(false);
            });

        SetColors(false);
    }

    void SetHighlighted(bool isHighlighted)
    {
        mIsHighlighted = isHighlighted;
        SetColors(false); // Assuming we're not focused now
    }

private:

    void SetColors(bool isFocused)
    {
        if (!mIsHighlighted)
        {
            if (!isFocused)
            { 
                SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
                SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
            }
            else
            {
                SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT));
                SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
            }
        }
        else
        {
            if (!isFocused)
            {
                SetBackgroundColour(mBackgroundUnfocusedHighlightColor);
                SetForegroundColour(mForegroundUnfocusedHighlightColor);
            }
            else
            {
                SetBackgroundColour(mBackgroundFocusedHighlightColor);
                SetForegroundColour(mForegroundFocusedHighlightColor);
            }
        }
    }

    wxColor const mBackgroundUnfocusedHighlightColor;
    wxColor const mForegroundUnfocusedHighlightColor;
    wxColor const mBackgroundFocusedHighlightColor;
    wxColor const mForegroundFocusedHighlightColor;

    bool mIsHighlighted;
};
