/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <UILib/ScalarTimeSeriesProbeControl.h>
#include <UILib/UnFocusablePanel.h>

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>

#include <wx/sizer.h>
#include <wx/wx.h>

#include <memory>
#include <string>
#include <unordered_map>

class ProbePanel final
    : public UnFocusablePanel
    , public ILifecycleGameEventHandler
    , public IStatisticsGameEventHandler
	, public IAtmosphereGameEventHandler
    , public IGenericGameEventHandler
{
public:

    ProbePanel(wxWindow* parent);

    virtual ~ProbePanel();

    void UpdateSimulation();

public:

    //
    // Game event handlers
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterStatisticsEventHandler(this);
		gameController.RegisterAtmosphereEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
    }

    void OnGameReset() override;

    void OnWaterTaken(float waterTaken) override;

    void OnWindSpeedUpdated(
        float const zeroSpeedMagnitude,
        float const baseSpeedMagnitude,
        float const baseAndStormSpeedMagnitude,
        float const preMaxSpeedMagnitude,
        float const maxSpeedMagnitude,
        vec2f const & windSpeed) override;

    void OnCustomProbe(
        std::string const & name,
        float value) override;

    void OnFrameRateUpdated(
        float immediateFps,
        float averageFps) override;

    void OnCurrentUpdateDurationUpdated(float currentUpdateDuration) override;

    void OnStaticPressureUpdated(
        float netForce,
        float complexity) override;

private:

    bool IsActive() const
    {
        return this->IsShown();
    }

    std::unique_ptr<ScalarTimeSeriesProbeControl> AddScalarTimeSeriesProbe(
        wxString const & name,
        int sampleCount);

private:

    //
    // Probes
    //

    wxBoxSizer * mProbesSizer;

    std::unique_ptr<ScalarTimeSeriesProbeControl> mFrameRateProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mCurrentUpdateDurationProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mWaterTakenProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mWindSpeedProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mStaticPressureNetForceProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mStaticPressureComplexityProbe;
    std::unordered_map<std::string, std::unique_ptr<ScalarTimeSeriesProbeControl>> mCustomProbes;
};
