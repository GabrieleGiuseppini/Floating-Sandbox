/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-08-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <wx/dcbuffer.h>
#include <wx/wx.h>

#include <array>
#include <chrono>
#include <memory>

class CreditsPanel : public wxPanel
{
public:

    CreditsPanel(wxWindow* parent);

private:

    struct Title;

    void RenderCredits(wxSize panelSize);
    void RenderTitle(Title const & title, int centerX, int & currentY, wxDC & dc, bool doRender);

    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);
    void OnLeftDown(wxMouseEvent & event);
    void OnMouseMove(wxMouseEvent & event);
    void OnScrollTimer(wxTimerEvent & event);

private:

    struct Title
    {
        int FontIndex;
        wxString Text;
        int BottomMargin;
    };

private:

    std::array<wxFont, 4> mFonts;

    std::unique_ptr<wxBitmap> mCreditsBitmap;
    std::unique_ptr<wxBufferedPaintDC> mCreditsBitmapBufferedPaintDC;
    int mMaxScrollOffsetY;

    std::unique_ptr<wxTimer> mScrollTimer;

    // State
    std::chrono::steady_clock::time_point mStartTimestamp;
    int mCurrentScrollOffsetY;
    wxPoint mLastMousePosition;
};
