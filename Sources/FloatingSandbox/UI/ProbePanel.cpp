/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ProbePanel.h"

#include <wx/stattext.h>

#include <cassert>
#include <chrono>

static constexpr int TopPadding = 2;
static constexpr int ProbePadding = 10;

ProbePanel::ProbePanel(wxWindow* parent)
    : UnFocusablePanel(
        parent,
        wxBORDER_SIMPLE | wxCLIP_CHILDREN)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));


    //
    // Create probes
    //

    mProbesSizer = new wxBoxSizer(wxHORIZONTAL);

    mFrameRateProbe = AddProbe<ScalarTimeSeriesProbeControl>(_("Frame Rate"), 150);
    mCurrentUpdateDurationProbe = AddProbe<ScalarTimeSeriesProbeControl>(_("Update Time"), 150);

    mWaterTakenProbe = AddProbe<ScalarTimeSeriesProbeControl>(_("Water Inflow"), 120);

    mWindSpeedProbe = AddProbe<ScalarTimeSeriesProbeControl>(_("Wind Speed"), 120);

    mStaticPressureNetForceProbe = AddProbe<ScalarTimeSeriesProbeControl>(_("Static Pressure Net Force"), 120);
    mStaticPressureComplexityProbe = AddProbe<ScalarTimeSeriesProbeControl>(_("Static Pressure Complexity"), 120);

    //
    // Finalize
    //

    SetSizerAndFit(mProbesSizer);
}

ProbePanel::~ProbePanel()
{
}

void ProbePanel::UpdateSimulation()
{
    //
    // Update all probes
    //

    if (IsActive())
    {
        mFrameRateProbe->UpdateSimulation();
        mCurrentUpdateDurationProbe->UpdateSimulation();
        mWaterTakenProbe->UpdateSimulation();
        mWindSpeedProbe->UpdateSimulation();
        mStaticPressureNetForceProbe->UpdateSimulation();
        mStaticPressureComplexityProbe->UpdateSimulation();

        if (mPressureCrossCutReadingsProbe)
        {
            mPressureCrossCutReadingsProbe->UpdateSimulation();
        }

        for (auto const & p : mCustomProbes)
        {
            p.second->UpdateSimulation();
        }
    }
}

template<typename TProbeControl>
std::unique_ptr<TProbeControl> ProbePanel::AddProbe(
    wxString const & name,
    int sampleCount)
{
    wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

    sizer->AddSpacer(TopPadding);

    auto probe = std::make_unique<TProbeControl>(this, sampleCount);
    sizer->Add(probe.get(), 1, wxALIGN_CENTRE, 0);

    wxStaticText * label = new wxStaticText(this, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(label, 0, wxALIGN_CENTRE, 0);

    mProbesSizer->Add(sizer, 1, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, ProbePadding);

    return probe;
}

///////////////////////////////////////////////////////////////////////////////////////

void ProbePanel::OnGameReset()
{
    mFrameRateProbe->Reset();
    mCurrentUpdateDurationProbe->Reset();
    mWaterTakenProbe->Reset();
    mWindSpeedProbe->Reset();
    mStaticPressureNetForceProbe->Reset();
    mStaticPressureComplexityProbe->Reset();

    if (mPressureCrossCutReadingsProbe)
    {
        mPressureCrossCutReadingsProbe->Reset();
    }

    for (auto const & p : mCustomProbes)
    {
        p.second->Reset();
    }
}

void ProbePanel::OnWaterTaken(float waterTaken)
{
    mWaterTakenProbe->RegisterSample(waterTaken);
}

void ProbePanel::OnWindSpeedUpdated(
    float const /*zeroSpeedMagnitude*/,
    float const /*baseSpeedMagnitude*/,
    float const /*baseAndStormSpeedMagnitude*/,
    float const /*preMaxSpeedMagnitude*/,
    float const /*maxSpeedMagnitude*/,
    vec2f const & windSpeed)
{
    mWindSpeedProbe->RegisterSample(windSpeed.length());
}

void ProbePanel::OnCustomProbe(
    std::string const & name,
    float value)
{
    auto & probe = mCustomProbes[name];
    if (!probe)
    {
        probe = AddProbe<ScalarTimeSeriesProbeControl>(name, 100);
        mProbesSizer->Layout();
        SetSizerAndFit(mProbesSizer);
    }

    probe->RegisterSample(value);
}

void ProbePanel::OnFrameRateUpdated(
    float immediateFps,
    float /*averageFps*/)
{
    mFrameRateProbe->RegisterSample(immediateFps);
}

void ProbePanel::OnCurrentUpdateDurationUpdated(float currentUpdateDuration)
{
    mCurrentUpdateDurationProbe->RegisterSample(currentUpdateDuration);
}

void ProbePanel::OnStaticPressureUpdated(
    float netForce,
    float complexity)
{
    mStaticPressureNetForceProbe->RegisterSample(netForce);
    mStaticPressureComplexityProbe->RegisterSample(complexity);
}

void ProbePanel::OnPressureReadings(std::vector<PressureReading> const & pressureReadings)
{
    if (!mPressureCrossCutReadingsProbe)
    {
        mPressureCrossCutReadingsProbe = AddProbe<PressureCrossCutReadingsProbeControl>(_("Pressure Profile"), 450);
        mProbesSizer->Layout();
        SetSizerAndFit(mProbesSizer);
    }

    mPressureCrossCutReadingsProbe->RegisterReadings(pressureReadings);
}
