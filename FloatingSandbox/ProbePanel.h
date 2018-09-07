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

    virtual void OnWaterSplashed(float waterSplashed) override;

private:

    bool IsActive() const
    {
        return this->IsShown();
    }

private:

    //
    // Probes
    //

    std::unique_ptr<ScalarTimeSeriesProbeControl> mWaterSplashProbe;
};
