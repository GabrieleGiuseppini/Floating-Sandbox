/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LoggingDialog.h"

#include <GameCore/Log.h>

#include <wx/clipbrd.h>
#include <wx/settings.h>

#include <cassert>

wxBEGIN_EVENT_TABLE(LoggingDialog, wxDialog)
	EVT_CLOSE(LoggingDialog::OnClose)
wxEND_EVENT_TABLE()

LoggingDialog::LoggingDialog(wxWindow * parent)
	: mParent(parent)
{
	Create(
		mParent,
		wxID_ANY,
		_("Logging"),
		wxDefaultPosition,
        wxSize(800, 250),
		wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxFRAME_SHAPED,
		_T("Logging Window"));

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
    // Connect key events
    //

    this->Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(LoggingDialog::OnKeyDown), (wxObject*)NULL, this);
    mTextCtrl->Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(LoggingDialog::OnKeyDown), (wxObject*)NULL, this);
}

LoggingDialog::~LoggingDialog()
{
}

void LoggingDialog::Open()
{
	Logger::Instance.RegisterListener(
		[this](std::string const & message)
		{
			assert(this->mTextCtrl != nullptr);
			this->mTextCtrl->WriteText(message);
		});

	this->Show();
}

void LoggingDialog::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == 'C')
    {
        // Copy content to clipboard
        if (wxTheClipboard->Open())
        {
            wxTheClipboard->Clear();
            wxTheClipboard->SetData(new wxTextDataObject(this->mTextCtrl->GetValue()));
            wxTheClipboard->Flush();
            wxTheClipboard->Close();
        }
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
