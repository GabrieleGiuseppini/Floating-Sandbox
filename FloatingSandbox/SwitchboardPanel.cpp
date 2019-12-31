/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SwitchboardPanel.h"

#include <cassert>
#include <utility>

std::unique_ptr<SwitchboardPanel> SwitchboardPanel::Create(
    wxWindow * parent,
    std::shared_ptr<IGameController> gameController,
    ResourceLoader & resourceLoader)
{
    return std::unique_ptr<SwitchboardPanel>(
        new SwitchboardPanel(
            parent,
            gameController,
            resourceLoader));
}

SwitchboardPanel::SwitchboardPanel(
    wxWindow * parent,
    std::shared_ptr<IGameController> gameController,
    ResourceLoader & resourceLoader)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE)
    , mSwitchMap()
    , mGameController(gameController)
{
    // TODOTEST
    //SetDoubleBuffered(true);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    //
    // Make panel
    //

    MakePanel();

    //
    // Load bitmaps
    //

    mInteractiveSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
}

SwitchboardPanel::~SwitchboardPanel()
{
}

void SwitchboardPanel::MakePanel()
{
    // Create grid sizer for switch panel
    mSwitchSizer = new wxFlexGridSizer(1, 0, 0, 0);
    mSwitchSizer->SetFlexibleDirection(wxHORIZONTAL);

    // Create panel for switches
    mSwitchPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    mSwitchPanel->SetSizerAndFit(mSwitchSizer);
}

///////////////////////////////////////////////////////////////////////////////////////

void SwitchboardPanel::OnGameReset()
{
    // Reset all controls
    mSwitchPanel->Destroy();
    mSwitchPanel = nullptr;
    mSwitchSizer = nullptr;
    MakePanel();

    // Clear map
    mSwitchMap.clear();
}

void SwitchboardPanel::OnSwitchCreated(
    SwitchId switchId,
    std::string const & name,
    SwitchType type,
    SwitchState state)
{
    LogMessage("TODOTEST: SwitchboardPanel::OnSwitchCreated: ", name, " T=", int(type));

    // TODO: handle overflow, add row eventually

    //
    // Create control
    //

    wxPanel * ctrl;
    switch (type)
    {
        case SwitchType::Interactive:
        {
            ctrl = new ShipInteractiveSwitchControl(
                mSwitchPanel,
                mInteractiveSwitchOnEnabledBitmap,
                mInteractiveSwitchOffEnabledBitmap,
                mInteractiveSwitchOnDisabledBitmap,
                mInteractiveSwitchOffDisabledBitmap,
                switchId,
                name,
                [this](SwitchId switchId, SwitchState newState)
                {
                    this->mGameController->SetSwitchState(switchId, newState);
                },
                state);

            break;
        }

        case SwitchType::WaterSensing:
        {
            // TODO
            ctrl = nullptr;
            return;

            break;
        }
    }

    mSwitchSizer->Add(
        ctrl,
        0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

    // TODOTEST: figure out which one is needed

    mSwitchPanel->Layout();
    mSwitchPanel->Fit();

    this->Layout();
    this->Fit();

    ////this->GetParent()->Layout();


    //
    // Add switch to map
    //

    assert(mSwitchMap.find(switchId) == mSwitchMap.end());
    mSwitchMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(switchId),
        std::forward_as_tuple(type, ctrl));
}

void SwitchboardPanel::OnSwitchEnabled(
    SwitchId switchId,
    bool isEnabled)
{
    auto it = mSwitchMap.find(switchId);
    assert(it != mSwitchMap.end());

    // Enable/disable control
    switch (it->second.Type)
    {
        case SwitchType::Interactive:
        {
            auto * ctrl = dynamic_cast<ShipInteractiveSwitchControl *>(it->second.SwitchControl);
            ctrl->SetEnabled(isEnabled);
            break;
        }

        case SwitchType::WaterSensing:
        {
            auto * ctrl = dynamic_cast<ShipAutomaticSwitchControl *>(it->second.SwitchControl);
            ctrl->SetEnabled(isEnabled);
            break;
        }
    }

    // Remember enable state
    it->second.IsEnabled = isEnabled;
}

void SwitchboardPanel::OnSwitchToggled(
    SwitchId switchId,
    SwitchState newState)
{
    auto it = mSwitchMap.find(switchId);
    assert(it != mSwitchMap.end());

    // Toggle control
    switch (it->second.Type)
    {
        case SwitchType::Interactive:
        {
            auto * ctrl = dynamic_cast<ShipInteractiveSwitchControl *>(it->second.SwitchControl);
            ctrl->SetState(newState);
            break;
        }

        case SwitchType::WaterSensing:
        {
            auto * ctrl = dynamic_cast<ShipAutomaticSwitchControl *>(it->second.SwitchControl);
            ctrl->SetState(newState);
            break;
        }
    }
}