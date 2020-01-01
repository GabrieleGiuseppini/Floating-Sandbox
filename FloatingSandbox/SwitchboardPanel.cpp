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
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    ResourceLoader & resourceLoader)
{
    return std::unique_ptr<SwitchboardPanel>(
        new SwitchboardPanel(
            parent,
            parentLayoutWindow,
            parentLayoutSizer,
            gameController,
            resourceLoader));
}

SwitchboardPanel::SwitchboardPanel(
    wxWindow * parent,
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    ResourceLoader & resourceLoader)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE)
    , mShowingMode(ShowingMode::NotShowing)
    , mSwitchMap()
    , mGameController(gameController)
    , mParentLayoutWindow(parentLayoutWindow)
    , mParentLayoutSizer(parentLayoutSizer)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    // TODOTEST
    SetCursor(wxNullCursor);

    mMainVSizer = new wxBoxSizer(wxVERTICAL);

    // Hint panel - hidden
    mHintPanel = new wxPanel(this);
    {
        wxBoxSizer * hintSizer = new wxBoxSizer(wxVERTICAL);
        wxStaticText * hintStaticText = new wxStaticText(mHintPanel, wxID_ANY, "Switches", wxDefaultPosition, wxDefaultSize, 0);
        hintSizer->Add(hintStaticText, 0, wxALIGN_CENTER_HORIZONTAL);
        mHintPanel->SetSizer(hintSizer);
    }
    mMainVSizer->Add(mHintPanel, 0, wxALIGN_CENTER_HORIZONTAL);
    mMainVSizer->Hide(mHintPanel);

    // Switch panel - hidden
    MakeSwitchPanel();
    mMainVSizer->Hide(mSwitchPanel);

    SetSizer(mMainVSizer);

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

void SwitchboardPanel::Hide()
{
    LogMessage("TODOTEST:SwitchboardPanel::Hide()");

    // Hide hint panel
    mMainVSizer->Hide(mHintPanel);

    // Hide switch panel
    mMainVSizer->Hide(mSwitchPanel);

    // Update layout
    mMainVSizer->Layout();

    mShowingMode = ShowingMode::NotShowing;

    // Notify parent
    LayoutParent();
}

void SwitchboardPanel::ShowPartially()
{
    LogMessage("TODOTEST:SwitchboardPanel::ShowPartially()");

    // Show hint panel
    mMainVSizer->Show(mHintPanel);

    // Hide switch panel
    mMainVSizer->Hide(mSwitchPanel);

    // Update layout
    mMainVSizer->Layout();

    mShowingMode = ShowingMode::ShowingHint;

    // Notify parent
    LayoutParent();
}

void SwitchboardPanel::ShowFullyFloating()
{
    LogMessage("TODOTEST:SwitchboardPanel::ShowFullyFloating()");

    // Show hint panel
    mMainVSizer->Show(mHintPanel);

    // Show switch panel
    mMainVSizer->Show(mSwitchPanel);

    // Update layout
    mMainVSizer->Layout();

    mShowingMode = ShowingMode::ShowingFullyFloating;

    // Notify parent
    LayoutParent();
}

void SwitchboardPanel::ShowFullyDocked()
{
    LogMessage("TODOTEST:SwitchboardPanel::ShowFullyDocked()");

    // Show hint panel
    mMainVSizer->Show(mHintPanel);

    // Show switch panel
    mMainVSizer->Show(mSwitchPanel);

    // Update layout
    mMainVSizer->Layout();

    mShowingMode = ShowingMode::ShowingFullyDocked;

    // Notify parent
    LayoutParent();
}

void SwitchboardPanel::MakeSwitchPanel()
{
    // Create grid sizer for switch panel
    mSwitchPanelSizer = new wxFlexGridSizer(1, 0, 0, 0);
    mSwitchPanelSizer->SetFlexibleDirection(wxHORIZONTAL);

    // Create panel for switches
    mSwitchPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    mSwitchPanel->SetSizerAndFit(mSwitchPanelSizer);

    // Add switch panel to v-sizer
    mMainVSizer->Add(mSwitchPanel, 0, wxALIGN_CENTER_HORIZONTAL);
}

void SwitchboardPanel::LayoutParent()
{
    mParentLayoutWindow->Layout();
}

///////////////////////////////////////////////////////////////////////////////////////

void SwitchboardPanel::OnGameReset()
{
    LogMessage("TODOTEST:SwitchboardPanel::OnGameReset()");

    // Reset all switch controls
    mSwitchPanel->Destroy();
    mSwitchPanel = nullptr;
    mSwitchPanelSizer = nullptr;
    MakeSwitchPanel();

    // Hide
    Hide();

    // Clear switch map
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

    ShipSwitchControl * ctrl;
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

    LogMessage("TODOTEST:SwitchboardPanel::OnSwitchCreated: BEFORE: SwitchPanelSize=",
        mSwitchPanel->GetSize().GetX(), "x", mSwitchPanel->GetSize().GetY(),
        " ctrl Size=", ctrl->GetSize().GetX(), "x", ctrl->GetSize().GetY());

    // Add to sizer
    mSwitchPanelSizer->Add(
        ctrl,
        0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

    // Ask sizer to resize panel accordingly
    mSwitchPanelSizer->SetSizeHints(mSwitchPanel);

    mMainVSizer->Layout();

    LayoutParent();

    LogMessage("TODOTEST:SwitchboardPanel::OnSwitchCreated: AFTER: SwitchPanelSize=",
        mSwitchPanel->GetSize().GetX(), "x", mSwitchPanel->GetSize().GetY(),
        " visible=", mSwitchPanel->IsShown());


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
    // Enable/disable switch control
    auto it = mSwitchMap.find(switchId);
    assert(it != mSwitchMap.end());
    it->second.SwitchControl->SetEnabled(isEnabled);

    // Remember enable state
    it->second.IsEnabled = isEnabled;
}

void SwitchboardPanel::OnSwitchToggled(
    SwitchId switchId,
    SwitchState newState)
{
    // Toggle switch control
    auto it = mSwitchMap.find(switchId);
    assert(it != mSwitchMap.end());
    it->second.SwitchControl->SetState(newState);
}