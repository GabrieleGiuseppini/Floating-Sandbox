/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LoggingDialog.h"

#include <Core/Log.h>

#include <wx/clipbrd.h>
#include <wx/settings.h>

#include <cassert>
#include <chrono>

wxBEGIN_EVENT_TABLE(LoggingDialog, wxDialog)
	EVT_CLOSE(LoggingDialog::OnClose)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(fsEVT_LOG_MESSAGE, fsLogMessageEvent);

LoggingDialog::LoggingDialog(wxWindow * parent)
	: mParent(parent)
{
	Create(
		mParent,
		wxID_ANY,
		_("Logging"),
		wxDefaultPosition,
        wxSize(600, 600),
		wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxFRAME_SHAPED,
		wxS("Logging Window"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	//
	// Create Text control
	//

	mTextCtrl = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		wxSize(200, 200),
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxVSCROLL | wxHSCROLL | wxBORDER_NONE);

	wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	mTextCtrl->SetFont(font);

    //
    // Connect events
    //

    Bind(wxEVT_KEY_DOWN, &LoggingDialog::OnKeyDown, this);
    mTextCtrl->Bind(wxEVT_KEY_DOWN, &LoggingDialog::OnKeyDown, this);
    Bind(fsEVT_LOG_MESSAGE, &LoggingDialog::OnLogMessage, this);
}

LoggingDialog::~LoggingDialog()
{
    Logger::Instance.UnregisterListener();
}

void LoggingDialog::Open()
{
    if (!this->IsShown())
    {
        Logger::Instance.RegisterListener(
            [this, windowId = this->GetId()](std::string const & message)
        {
            fsLogMessageEvent * event = new fsLogMessageEvent(fsEVT_LOG_MESSAGE, windowId, message);
            QueueEvent(event);
        });

        this->Show();
    }
}

void LoggingDialog::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == 'C')
    {
        //
        // Copy content to clipboard
        //

        if (wxTheClipboard->Open())
        {
            wxTheClipboard->Clear();
            wxTheClipboard->SetData(new wxTextDataObject(this->mTextCtrl->GetValue()));
            wxTheClipboard->Flush();
            wxTheClipboard->Close();
        }
    }
    else if (event.GetKeyCode() == 'L')
    {
        //
        // Log a marker
        //

        LogMessage("-------------------- ");
    }
    else if (event.GetKeyCode() == 'X')
    {
        //
        // Clear
        //

        assert(this->mTextCtrl != nullptr);
        this->mTextCtrl->Clear();
    }
}

void LoggingDialog::OnClose(wxCloseEvent & event)
{
	Logger::Instance.UnregisterListener();

	// Be nice, clear the control
	assert(this->mTextCtrl != nullptr);
	this->mTextCtrl->Clear();

	event.Skip();
}

void LoggingDialog::OnLogMessage(fsLogMessageEvent & event)
{
    assert(this->mTextCtrl != nullptr);
    this->mTextCtrl->WriteText(event.GetMessage());
}