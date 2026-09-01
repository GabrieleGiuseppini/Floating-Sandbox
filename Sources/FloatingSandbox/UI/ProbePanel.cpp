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

    wxPen const defaultPen = wxPen(wxColor("BLACK"), 2, wxPENSTYLE_SOLID);

    mProbesSizer = new wxBoxSizer(wxHORIZONTAL);

    mFrameRateProbe = AddProbe<ScalarTimeSeriesProbeControl<float>>(_("Frame Rate"), 150);
    mFrameRateProbe->SetPens({ defaultPen });
    mCurrentUpdateDurationProbe = AddProbe<ScalarTimeSeriesProbeControl<float>>(_("Update Time"), 150);
    mCurrentUpdateDurationProbe->SetPens({ defaultPen });

    mPressureIntakeProbe = AddProbe<ScalarTimeSeriesProbeControl<float, float>>(_("Water Inflow"), 120);
    mPressureIntakeProbe->SetPens({ wxPen(wxColor("BLUE"), 2, wxPENSTYLE_SOLID), wxPen(wxColor("PINK"), 2, wxPENSTYLE_SOLID) });

    mWindSpeedProbe = AddProbe<ScalarTimeSeriesProbeControl<float>>(_("Wind Speed"), 120);
    mWindSpeedProbe->SetPens({ defaultPen });

    mStaticPressureNetForceProbe = AddProbe<ScalarTimeSeriesProbeControl<float>>(_("Static Pressure Net Force"), 120);
    mStaticPressureNetForceProbe->SetPens({ defaultPen });
    mStaticPressureComplexityProbe = AddProbe<ScalarTimeSeriesProbeControl<float>>(_("Static Pressure Complexity"), 120);
    mStaticPressureComplexityProbe->SetPens({ defaultPen });

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
        mPressureIntakeProbe->UpdateSimulation();
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
    mPressureIntakeProbe->Reset();
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

void ProbePanel::OnPressureIntake(
    float waterTaken,
    float airTaken)
{
    mPressureIntakeProbe->RegisterSample({ waterTaken, airTaken });
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
        probe = AddProbe<ScalarTimeSeriesProbeControl<float>>(name, 100);
        probe->SetPens({ wxPen(wxColor("BLACK"), 2, wxPENSTYLE_SOLID) });
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
