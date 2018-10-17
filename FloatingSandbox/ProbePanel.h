/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ScalarTimeSeriesProbeControl.h"

#include <GameLib/IGameEventHandler.h>

#include <wx/wx.h>

#include <memory>
#include <string>
#include <unordered_map>

class ProbePanel : public wxPanel, public IGameEventHandler
{
public:

    ProbePanel(wxWindow* parent);
    
    virtual ~ProbePanel();

    void Update();

public:

    //
    // IGameEventHandler events
    //

    virtual void OnGameReset() override;

    virtual void OnWaterTaken(float waterTaken) override;

    virtual void OnWaterSplashed(float waterSplashed) override;

    virtual void OnCustomProbe(
        std::string const & name,
        float value) override;

    virtual void OnFrameRateUpdated(
        float immediateFps,
        float averageFps) override;

private:

    bool IsActive() const
    {
        return this->IsShown();
    }

private:

    //
    // Probes
    //

    std::unique_ptr<ScalarTimeSeriesProbeControl> mFrameRateProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mWaterTakenProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mWaterSplashProbe;
    std::unordered_map<std::string, std::unique_ptr<ScalarTimeSeriesProbeControl>> mCustomProbes;
};
