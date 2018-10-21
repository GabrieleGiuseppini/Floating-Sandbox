/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ProbePanel.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

#include <cassert>

static constexpr int TopPadding = 2;
static constexpr int ProbePadding = 10;

ProbePanel::ProbePanel(wxWindow* parent)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition, 
        wxDefaultSize,
        wxBORDER_SIMPLE | wxCLIP_CHILDREN)
{
    SetDoubleBuffered(true);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    

    //
    // Create probes
    //

    wxBoxSizer * probesSizer = new wxBoxSizer(wxHORIZONTAL);

    {
        wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

        sizer->AddSpacer(TopPadding);

        mFrameRateProbe = std::make_unique<ScalarTimeSeriesProbeControl>(this, 200);
        sizer->Add(mFrameRateProbe.get(), 1, wxALIGN_CENTRE, 0);

        wxStaticText * label = new wxStaticText(this, wxID_ANY, "Frame Rate", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
        sizer->Add(label, 0, wxALIGN_CENTRE, 0);

        probesSizer->Add(sizer, 1, wxLEFT | wxRIGHT, ProbePadding);
    }

    {
        wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

        sizer->AddSpacer(TopPadding);

        mWaterTakenProbe = std::make_unique<ScalarTimeSeriesProbeControl>(this, 120);
        sizer->Add(mWaterTakenProbe.get(), 1, wxALIGN_CENTRE, 0);        

        wxStaticText * label = new wxStaticText(this, wxID_ANY, "Water Inflow", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
        sizer->Add(label, 0, wxALIGN_CENTRE, 0);

        probesSizer->Add(sizer, 1, wxLEFT | wxRIGHT, ProbePadding);
    }

    {
        wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

        sizer->AddSpacer(TopPadding);

        mWaterSplashProbe = std::make_unique<ScalarTimeSeriesProbeControl>(this, 200);
        sizer->Add(mWaterSplashProbe.get(), 1, wxALIGN_CENTRE, 0);

        wxStaticText * label = new wxStaticText(this, wxID_ANY, "Water Splash", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
        sizer->Add(label, 0, wxALIGN_CENTRE, 0);

        probesSizer->Add(sizer, 1, wxLEFT | wxRIGHT, ProbePadding);
    }

    //
    // Finalize
    //

    SetSizerAndFit(probesSizer);
}

ProbePanel::~ProbePanel()
{
}

void ProbePanel::Update()
{
    //
    // Update all probes
    //

    if (IsActive())
    {
        mFrameRateProbe->Update();
        mWaterTakenProbe->Update();
        mWaterSplashProbe->Update();
    }
}

///////////////////////////////////////////////////////////////////////////////////////

void ProbePanel::OnGameReset()
{
    mFrameRateProbe->Reset();
    mWaterTakenProbe->Reset();
    mWaterSplashProbe->Reset();
}

void ProbePanel::OnWaterTaken(float waterTaken)
{
    mWaterTakenProbe->RegisterSample(waterTaken);
}

void ProbePanel::OnWaterSplashed(float waterSplashed)
{
    mWaterSplashProbe->RegisterSample(waterSplashed);
}

void ProbePanel::OnCustomProbe(
    std::string const & name,
    float value)
{
    // TODO
}

void ProbePanel::OnFrameRateUpdated(
    float immediateFps,
    float /*averageFps*/)
{
    mFrameRateProbe->RegisterSample(immediateFps);
}
