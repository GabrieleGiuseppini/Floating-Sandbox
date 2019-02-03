/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>

#include <memory>

class fsLogMessageEvent : public wxEvent
{
public:

    fsLogMessageEvent(
        wxEventType eventType,
        int winid,
        std::string const & message)
        : wxEvent(winid, eventType)
        , mMessage(message)
    {
    }

    fsLogMessageEvent(fsLogMessageEvent const & other)
        : wxEvent(other)
        , mMessage(other.mMessage)
    {
    }

    virtual wxEvent *Clone() const override
    {
        return new fsLogMessageEvent(*this);
    }

    std::string const & GetMessage() const
    {
        return mMessage;
    }

private:
    std::string const mMessage;
};

wxDECLARE_EVENT(fsEVT_LOG_MESSAGE, fsLogMessageEvent);


class LoggingDialog : public wxDialog
{
public:

	LoggingDialog(wxWindow* parent);

	virtual ~LoggingDialog();

	void Open();

private:

    void OnKeyDown(wxKeyEvent& event);
	void OnClose(wxCloseEvent& event);
    void OnLogMessage(fsLogMessageEvent & event);

private:

	wxWindow * const mParent;

	wxTextCtrl * mTextCtrl;

	DECLARE_EVENT_TABLE()
};
