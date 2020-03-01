/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SwitchboardPanel.h"

#include "WxHelpers.h"

#include <GameCore/ImageTools.h>

#include <Game/GameParameters.h>
#include <Game/ImageFileTools.h>

#include <UIControls/LayoutHelper.h>

#include <wx/clntdata.h>
#include <wx/cursor.h>

#include <algorithm>
#include <cassert>
#include <utility>

static constexpr int MaxElementsPerRow = 11;
static constexpr int MaxKeyboardShortcuts = 20;

std::unique_ptr<SwitchboardPanel> SwitchboardPanel::Create(
    wxWindow * parent,
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
    ResourceLoader & resourceLoader,
    ProgressCallback const & progressCallback)
{
    return std::unique_ptr<SwitchboardPanel>(
        new SwitchboardPanel(
            parent,
            parentLayoutWindow,
            parentLayoutSizer,
            std::move(gameController),
            std::move(soundController),
            std::move(uiPreferencesManager),
            resourceLoader,
            progressCallback));
}

SwitchboardPanel::SwitchboardPanel(
    wxWindow * parent,
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
    ResourceLoader & resourceLoader,
    ProgressCallback const & progressCallback)
    : mShowingMode(ShowingMode::NotShowing)
    , mLeaveWindowTimer()
    , mBackgroundBitmapComboBox(nullptr)
    , mBackgroundSelectorPopup()
    //
    , mElementMap()
    , mUpdateableElements()
    , mKeyboardShortcutToElementId()
    , mCurrentKeyDownElementId()
    //
    , mGameController(std::move(gameController))
    , mSoundController(std::move(soundController))
    , mUIPreferencesManager(std::move(uiPreferencesManager))
    , mParentLayoutWindow(parentLayoutWindow)
    , mParentLayoutSizer(parentLayoutSizer)
    //
    , mMinBitmapSize(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())
{
    float constexpr TotalProgressSteps = 7.0f;
    float ProgressSteps = 0.0;

    wxPanel::Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE);

    //
    // Setup background selector popup
    //

    progressCallback(ProgressSteps/TotalProgressSteps, "Loading electrical panel...");

    auto backgroundBitmapFilepaths = resourceLoader.GetBitmapFilepaths("switchboard_background_*");
    if (backgroundBitmapFilepaths.empty())
    {
        throw GameException("There are no switchboard background bitmaps available");
    }

    std::sort(
        backgroundBitmapFilepaths.begin(),
        backgroundBitmapFilepaths.end(),
        [](auto const & fp1, auto const & fp2)
        {
            return fp1.string() < fp2.string();
        });


    mBackgroundSelectorPopup = std::make_unique<wxPopupTransientWindow>(this, wxBORDER_SIMPLE);
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        {
            mBackgroundBitmapComboBox = new wxBitmapComboBox(mBackgroundSelectorPopup.get(), wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxDefaultSize, wxArrayString(), wxCB_READONLY);

            for (auto const & backgroundBitmapFilepath : backgroundBitmapFilepaths)
            {
                auto backgroundBitmapThumb1 = ImageFileTools::LoadImageRgbaLowerLeftAndResize(
                    backgroundBitmapFilepath,
                    128);

                auto backgroundBitmapThumb2 = ImageTools::Truncate(std::move(backgroundBitmapThumb1), ImageSize(64, 32));

                mBackgroundBitmapComboBox->Append(
                    _(""),
                    WxHelpers::MakeBitmap(backgroundBitmapThumb2),
                    new wxStringClientData(backgroundBitmapFilepath.string()));
            }

            mBackgroundBitmapComboBox->Bind(wxEVT_COMBOBOX, &SwitchboardPanel::OnBackgroundSelectionChanged, this);

            sizer->Add(mBackgroundBitmapComboBox, 1, wxALL | wxEXPAND, 0);
        }

        mBackgroundSelectorPopup->SetSizerAndFit(sizer);
    }

    //
    // Set background bitmap
    //

    // Select background from preferences
    int backgroundBitmapIndex = std::min(
        mUIPreferencesManager->GetSwitchboardBackgroundBitmapIndex(),
        static_cast<int>(mBackgroundBitmapComboBox->GetCount()) - 1);
    mBackgroundBitmapComboBox->Select(backgroundBitmapIndex);

    // Set bitmap
    SetBackgroundBitmapFromCombo(mBackgroundBitmapComboBox->GetSelection());

    //
    // Setup cursor
    //

    // Load cursor
    auto upCursor = WxHelpers::LoadCursor(
        "switch_cursor_up",
        8,
        9,
        resourceLoader);

    // Set cursor
    SetCursor(upCursor);

    //
    // Load bitmaps
    //

    ProgressSteps += 1.0f; // 1.0f
    progressCallback(ProgressSteps / TotalProgressSteps, "Loading electrical panel...");

    mAutomaticSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("automatic_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mAutomaticSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("automatic_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mAutomaticSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("automatic_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mAutomaticSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("automatic_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mAutomaticSwitchOnEnabledBitmap.GetSize());

    ProgressSteps += 1.0f; // 2.0f
    progressCallback(ProgressSteps / TotalProgressSteps, "Loading electrical panel...");

    mInteractivePushSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_push_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_push_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_push_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_push_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mInteractivePushSwitchOnEnabledBitmap.GetSize());

    ProgressSteps += 1.0f; // 3.0f
    progressCallback(ProgressSteps / TotalProgressSteps, "Loading electrical panel...");

    mInteractiveToggleSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_toggle_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_toggle_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_toggle_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("interactive_toggle_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mInteractiveToggleSwitchOnEnabledBitmap.GetSize());

    ProgressSteps += 1.0f; // 4.0f
    progressCallback(ProgressSteps / TotalProgressSteps, "Loading electrical panel...");

    mPowerMonitorOnBitmap.LoadFile(resourceLoader.GetBitmapFilepath("power_monitor_on").string(), wxBITMAP_TYPE_PNG);
    mPowerMonitorOffBitmap.LoadFile(resourceLoader.GetBitmapFilepath("power_monitor_off").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mPowerMonitorOnBitmap.GetSize());

    ProgressSteps += 1.0f; // 5.0f
    progressCallback(ProgressSteps / TotalProgressSteps, "Loading electrical panel...");

    mGaugeRpmBitmap.LoadFile(resourceLoader.GetBitmapFilepath("gauge_rpm").string(), wxBITMAP_TYPE_PNG);
    mGaugeVoltsBitmap.LoadFile(resourceLoader.GetBitmapFilepath("gauge_volts").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mGaugeRpmBitmap.GetSize());

    ProgressSteps += 1.0f; // 6.0f
    progressCallback(ProgressSteps / TotalProgressSteps, "Loading electrical panel...");

    mEngineControllerBackgroundEnabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("telegraph_background_enabled").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerBackgroundDisabledBitmap.LoadFile(resourceLoader.GetBitmapFilepath("telegraph_background_disabled").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_0").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_1").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_2").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_3").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_4").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_5").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_6").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_7").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_8").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_9").string(), wxBITMAP_TYPE_PNG);
    mEngineControllerHandBitmaps.emplace_back(resourceLoader.GetBitmapFilepath("telegraph_hand_10").string(), wxBITMAP_TYPE_PNG);

    ProgressSteps += 1.0f; // 7.0f
    progressCallback(ProgressSteps / TotalProgressSteps, "Loading electrical panel...");

    wxBitmap dockCheckboxCheckedBitmap(resourceLoader.GetBitmapFilepath("electrical_panel_dock_pin_down").string(), wxBITMAP_TYPE_PNG);
    wxBitmap dockCheckboxUncheckedBitmap(resourceLoader.GetBitmapFilepath("electrical_panel_dock_pin_up").string(), wxBITMAP_TYPE_PNG);

    //
    // Setup panel
    //
    // HSizer1: |DockCheckbox(ShowToggable)| VSizer2 | Filler |
    //
    // VSizer2: ---------------
    //          |  HintPanel  |
    //          ---------------
    //          | SwitchPanel |
    //          ---------------

    mMainHSizer1 = new wxBoxSizer(wxHORIZONTAL);
    {
        // DockCheckbox
        {
            mDockCheckbox = new BitmappedCheckbox(this, wxID_ANY, dockCheckboxUncheckedBitmap, dockCheckboxCheckedBitmap, "Docks/Undocks the electrical panel.");
            mDockCheckbox->Bind(wxEVT_CHECKBOX, &SwitchboardPanel::OnDockCheckbox, this);

            mMainHSizer1->Add(mDockCheckbox, 0, wxALIGN_TOP, 0);
        }

        // VSizer2
        {
            mMainVSizer2 = new wxBoxSizer(wxVERTICAL);

            // Hint panel
            {
                mHintPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 16), 0);
                mHintPanel->SetMinSize(wxSize(-1, 16)); // Determines height of hint panel
                mHintPanel->Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);

                mMainVSizer2->Add(mHintPanel, 0, wxALIGN_CENTER_HORIZONTAL, 0);
            }

            // Switch panel
            {
                MakeSwitchPanel();

                // The panel adds itself to VSizer2
            }

            mMainHSizer1->Add(mMainVSizer2, 1, wxEXPAND, 0);
        }

        // Filler
        {
            mMainHSizer1->Add(dockCheckboxUncheckedBitmap.GetSize().GetWidth(), 1, 0, wxALIGN_TOP, 0);
        }
    }

    // Hide dock checkbox and filler now
    mMainHSizer1->Hide(size_t(0));
    mMainHSizer1->Hide(size_t(2));

    // Hide hint panel now
    mMainVSizer2->Hide(mHintPanel);

    // Hide switch panel now
    mMainVSizer2->Hide(mSwitchPanel);

    //
    // Set main sizer
    //

    SetSizer(mMainHSizer1);

    //
    // Setup mouse events
    //

    Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);

    mLeaveWindowTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mLeaveWindowTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&SwitchboardPanel::OnLeaveWindowTimer);

    Bind(wxEVT_RIGHT_DOWN, &SwitchboardPanel::OnRightDown, this);
}

SwitchboardPanel::~SwitchboardPanel()
{
}

void SwitchboardPanel::Update()
{
    for (auto ctrl : mUpdateableElements)
    {
        ctrl->Update();
    }
}

bool SwitchboardPanel::ProcessKeyDown(
    int keyCode,
    int keyModifiers)
{
    if (!!mCurrentKeyDownElementId)
    {
        // This is the subsequent in a sequence of key downs...
        // ...ignore it
        return false;
    }

    //
    // Translate key into index
    //

    int keyIndex;
    if (keyCode >= '1' && keyCode <= '9')
        keyIndex = keyCode - '1';
    else if (keyCode == '0')
        keyIndex = 9;
    else
        return false; // Not for us

    if (keyModifiers == wxMOD_CONTROL)
    {
        // 0-9
    }
    else if (keyModifiers == wxMOD_ALT)
    {
        // 10-19
        keyIndex += 10;
    }
    else
    {
        // Ignore
        return false;
    }

    //
    // Map and toggle
    //

    if (keyIndex < mKeyboardShortcutToElementId.size())
    {
        ElectricalElementId const elementId = mKeyboardShortcutToElementId[keyIndex];
        ElectricalElementInfo & elementInfo = mElementMap.at(elementId);
        if (nullptr == elementInfo.DisablableControl
            || elementInfo.DisablableControl->IsEnabled())
        {
            // Deliver event
            assert(nullptr != elementInfo.InteractiveControl);
            elementInfo.InteractiveControl->OnKeyboardShortcutDown();

            // Remember this is the first keydown
            mCurrentKeyDownElementId = elementId;

            // Processed
            return true;
        }
    }

    // Ignore
    return false;
}

bool SwitchboardPanel::ProcessKeyUp(
    int keyCode,
    int /*keyModifiers*/)
{
    if (!mCurrentKeyDownElementId)
        return false; // This is the subsequent in a sequence of key ups...

    // Check if it's a panel key
    if (keyCode < '0' || keyCode > '9')
        return false; // Not for the panel

    //
    // Map and toggle
    //

    ElectricalElementInfo & elementInfo = mElementMap.at(*mCurrentKeyDownElementId);
    if (nullptr == elementInfo.DisablableControl
        || elementInfo.DisablableControl->IsEnabled())
    {
        // Deliver event
        assert(nullptr != elementInfo.InteractiveControl);
        elementInfo.InteractiveControl->OnKeyboardShortcutUp();
    }

    // Remember this is the first keyup
    mCurrentKeyDownElementId.reset();

    // Processed
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////

void SwitchboardPanel::OnElectricalElementAnnouncementsBegin()
{
    // Stop refreshing - we'll resume when announcements are over
    Freeze();

    // Reset all switch controls
    mSwitchPanel->Destroy();
    mSwitchPanel = nullptr;
    mSwitchPanelSizer = nullptr;
    MakeSwitchPanel();

    // Clear maps
    mElementMap.clear();
    mUpdateableElements.clear();

    // Clear keyboard shortcuts map
    mKeyboardShortcutToElementId.clear();
    mCurrentKeyDownElementId.reset();
}

void SwitchboardPanel::OnSwitchCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    SwitchType type,
    ElectricalState state,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("SwitchboardPanel::OnSwitchCreated(", electricalElementId, " ", int(instanceIndex), "): State=", static_cast<bool>(state));

    //
    // Make label, if needed
    //

    std::string label;
    if (!!panelElementMetadata)
    {
        label = panelElementMetadata->Label;
    }
    else
    {
        // Make label
        std::stringstream ss;
        ss << "Switch " << " #" << static_cast<int>(instanceIndex);
        label = ss.str();
    }

    //
    // Make switch control
    //

    SwitchElectricalElementControl * swCtrl;
    IInteractiveElectricalElementControl * intCtrl;

    switch (type)
    {
        case SwitchType::InteractivePushSwitch:
        {
            auto ctrl = new InteractivePushSwitchElectricalElementControl(
                mSwitchPanel,
                mInteractivePushSwitchOnEnabledBitmap,
                mInteractivePushSwitchOffEnabledBitmap,
                mInteractivePushSwitchOnDisabledBitmap,
                mInteractivePushSwitchOffDisabledBitmap,
                label,
                [this, electricalElementId](ElectricalState newState)
                {
                    mGameController->SetSwitchState(electricalElementId, newState);
                },
                state);

            swCtrl = ctrl;
            intCtrl = ctrl;

            break;
        }

        case SwitchType::InteractiveToggleSwitch:
        {
            auto ctrl = new InteractiveToggleSwitchElectricalElementControl(
                mSwitchPanel,
                mInteractiveToggleSwitchOnEnabledBitmap,
                mInteractiveToggleSwitchOffEnabledBitmap,
                mInteractiveToggleSwitchOnDisabledBitmap,
                mInteractiveToggleSwitchOffDisabledBitmap,
                label,
                [this, electricalElementId](ElectricalState newState)
                {
                    mGameController->SetSwitchState(electricalElementId, newState);
                },
                state);

            swCtrl = ctrl;
            intCtrl = ctrl;

            break;
        }

        case SwitchType::AutomaticSwitch:
        {
            auto ctrl = new AutomaticSwitchElectricalElementControl(
                mSwitchPanel,
                mAutomaticSwitchOnEnabledBitmap,
                mAutomaticSwitchOffEnabledBitmap,
                mAutomaticSwitchOnDisabledBitmap,
                mAutomaticSwitchOffDisabledBitmap,
                label,
                state);

            swCtrl = ctrl;
            intCtrl = nullptr;

            break;
        }

        default:
        {
            assert(false);
            return;
        }
    }

    assert(swCtrl != nullptr);

    //
    // Add switch to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(swCtrl, swCtrl, intCtrl, panelElementMetadata));
}

void SwitchboardPanel::OnPowerProbeCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    PowerProbeType type,
    ElectricalState state,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("SwitchboardPanel::OnPowerProbeCreated(", electricalElementId, " ", int(instanceIndex), "): State=", static_cast<bool>(state));

    //
    // Create power monitor control
    //

    std::string label;
    ElectricalElementControl * ctrl;

    if (!!panelElementMetadata)
    {
        label = panelElementMetadata->Label;
    }

    switch (type)
    {
        case PowerProbeType::Generator:
        {
            // Voltage Gauge
            auto ggCtrl = new GaugeElectricalElementControl(
                mSwitchPanel,
                mGaugeVoltsBitmap,
                wxPoint(47, 47),
                36.0f,
                -Pi<float> / 4.0f,
                Pi<float> * 5.0f / 4.0f,
                label,
                state == ElectricalState::On ? 0.0f : 1.0f);

            ctrl = ggCtrl;

            // Store as updateable element
            mUpdateableElements.emplace_back(ggCtrl);

            if (!panelElementMetadata)
            {
                // Make label
                std::stringstream ss;
                ss << "Generator #" << static_cast<int>(instanceIndex);
                label = ss.str();
            }

            break;
        }

        case PowerProbeType::PowerMonitor:
        {
            ctrl = new PowerMonitorElectricalElementControl(
                mSwitchPanel,
                mPowerMonitorOnBitmap,
                mPowerMonitorOffBitmap,
                label,
                state);

            if (!panelElementMetadata)
            {
                // Make label
                std::stringstream ss;
                ss << "Monitor #" << static_cast<int>(instanceIndex);
                label = ss.str();
            }

            break;
        }

        default:
        {
            assert(false);
            return;
        }
    }

    assert(ctrl != nullptr);

    //
    // Add monitor to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(ctrl, nullptr, nullptr, panelElementMetadata));
}

void SwitchboardPanel::OnEngineControllerCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("SwitchboardPanel::OnEngineControllerCreated(", electricalElementId, " ", int(instanceIndex), ")");

    //
    // Create label
    //

    std::string label;
    if (!!panelElementMetadata)
    {
        label = panelElementMetadata->Label;
    }
    else
    {
        // Make label
        std::stringstream ss;
        ss << "EngineControl #" << static_cast<int>(instanceIndex);
        label = ss.str();
    }

    //
    // Create control
    //

    auto ecCtrl = new EngineControllerElectricalElementControl(
        mSwitchPanel,
        mEngineControllerBackgroundEnabledBitmap,
        mEngineControllerBackgroundDisabledBitmap,
        mEngineControllerHandBitmaps,
        wxPoint(47, 48),
        3.85f,
        -0.70f,
        label,
        [this, electricalElementId](unsigned int controllerValue)
        {
            mGameController->SetEngineControllerState(
                electricalElementId,
                controllerValue - static_cast<unsigned int>(mEngineControllerHandBitmaps.size() / 2));
        },
        mEngineControllerHandBitmaps.size() / 2); // Starting value = center

    //
    // Add to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(ecCtrl, ecCtrl, ecCtrl, panelElementMetadata));
}

void SwitchboardPanel::OnEngineMonitorCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    float thrustMagnitude,
    float rpm,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("SwitchboardPanel::OnEngineMonitorCreated(", electricalElementId, " ", int(instanceIndex), "): Thrust=", thrustMagnitude, ", RPM=", rpm);

    //
    // Create label
    //

    std::string label;
    if (!!panelElementMetadata)
    {
        label = panelElementMetadata->Label;
    }
    else
    {
        // Make label
        std::stringstream ss;
        ss << "Engine #" << static_cast<int>(instanceIndex);
        label = ss.str();
    }

    //
    // Create control
    //

    auto ggCtrl = new GaugeElectricalElementControl(
        mSwitchPanel,
        mGaugeRpmBitmap,
        wxPoint(47, 47),
        36.0f,
        Pi<float> / 4.0f - 0.06f,
        2.0f * Pi<float> - Pi<float> / 4.0f,
        label,
        1.0f - rpm);

    // Store as updateable element
    mUpdateableElements.emplace_back(ggCtrl);

    //
    // Add monitor to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(ggCtrl, nullptr, nullptr, panelElementMetadata));
}

void SwitchboardPanel::OnElectricalElementAnnouncementsEnd()
{
    //
    // Layout and assign keys
    //

    // Prepare elements for layout helper
    std::vector<LayoutHelper::LayoutElement<ElectricalElementId>> layoutElements;
    for (auto const it : mElementMap)
    {
        if (!!(it.second.PanelElementMetadata))
            layoutElements.emplace_back(
                it.first,
                std::make_pair(it.second.PanelElementMetadata->X, it.second.PanelElementMetadata->Y));
        else
            layoutElements.emplace_back(
                it.first,
                std::nullopt);
    }

    LayoutHelper::Layout<ElectricalElementId>(
        layoutElements,
        MaxElementsPerRow,
        [this](int width, int height)
        {
            mSwitchPanelSizer->SetCols(width);
            mSwitchPanelSizer->SetRows(height);
        },
        [this](std::optional<ElectricalElementId> elementId, int x, int y)
        {
            if (!!elementId)
            {
                // Get this element
                auto it = mElementMap.find(*elementId);
                assert(it != mElementMap.end());

                // Add control to sizer
                mSwitchPanelSizer->Add(
                    it->second.Control,
                    wxGBPosition(y, x + (mSwitchPanelSizer->GetCols() / 2)),
                    wxGBSpan(1, 1),
                    wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL | wxALIGN_BOTTOM,
                    8);

                // If interactive, make keyboard shortcut
                if (nullptr != it->second.InteractiveControl
                    && mKeyboardShortcutToElementId.size() < MaxKeyboardShortcuts)
                {
                    int keyIndex = static_cast<int>(mKeyboardShortcutToElementId.size());

                    // Store key mapping
                    mKeyboardShortcutToElementId.emplace_back(*elementId);

                    // Create shortcut label

                    std::stringstream ss;
                    if (keyIndex < 10)
                    {
                        ss << "Ctrl-";
                    }
                    else
                    {
                        assert(keyIndex < 20);
                        keyIndex -= 10;
                        ss << "Alt-";
                    }

                    assert(keyIndex >= 0 && keyIndex <= 9);
                    if (keyIndex == 9)
                        ss << "0";
                    else
                        ss << char('1' + keyIndex);

                    // Assign label
                    it->second.InteractiveControl->SetKeyboardShortcutLabel(ss.str());
                }
            }
        });

    // Ask sizer to resize panel accordingly
    mSwitchPanelSizer->SetSizeHints(mSwitchPanel);

    //
    // Decide panel visibility
    //

    if (mElementMap.empty())
    {
        // No elements

        // Hide
        HideFully();
    }
    else
    {
        // We have elements

        if (mUIPreferencesManager->GetAutoShowSwitchboard())
        {
            ShowFullyDocked();
        }
        else
        {
            ShowPartially();
        }
    }

    // Resume refresh
    Thaw();

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::OnSwitchEnabled(
    ElectricalElementId electricalElementId,
    bool isEnabled)
{
    //
    // Enable/disable switch
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Switch);

    SwitchElectricalElementControl * swCtrl = dynamic_cast<SwitchElectricalElementControl *>(elementInfo.Control);
    assert(swCtrl != nullptr);

    swCtrl->SetEnabled(isEnabled);
}

void SwitchboardPanel::OnSwitchToggled(
    ElectricalElementId electricalElementId,
    ElectricalState newState)
{
    //
    // Toggle switch
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Switch);

    SwitchElectricalElementControl * swCtrl = dynamic_cast<SwitchElectricalElementControl *>(elementInfo.Control);
    assert(swCtrl != nullptr);

    swCtrl->SetState(newState);
}

void SwitchboardPanel::OnPowerProbeToggled(
    ElectricalElementId electricalElementId,
    ElectricalState newState)
{
    //
    // Toggle control
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    if (elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::PowerMonitor)
    {
        PowerMonitorElectricalElementControl * pmCtrl = dynamic_cast<PowerMonitorElectricalElementControl *>(elementInfo.Control);
        assert(pmCtrl != nullptr);

        pmCtrl->SetState(newState);
    }
    else
    {
        assert(elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Gauge);

        GaugeElectricalElementControl * ggCtrl = dynamic_cast<GaugeElectricalElementControl *>(elementInfo.Control);
        assert(ggCtrl != nullptr);

        ggCtrl->SetValue(newState == ElectricalState::On ? 0.0f : 1.0f);
    }
}

void SwitchboardPanel::OnEngineControllerEnabled(
    ElectricalElementId electricalElementId,
    bool isEnabled)
{
    //
    // Enable/disable controller
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::EngineController);

    EngineControllerElectricalElementControl * ecCtrl = dynamic_cast<EngineControllerElectricalElementControl *>(elementInfo.Control);
    assert(ecCtrl != nullptr);

    ecCtrl->SetEnabled(isEnabled);
}

void SwitchboardPanel::OnEngineControllerUpdated(
    ElectricalElementId electricalElementId,
    int telegraphValue)
{
    //
    // Toggle switch
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::EngineController);

    EngineControllerElectricalElementControl * ecCtrl = dynamic_cast<EngineControllerElectricalElementControl *>(elementInfo.Control);
    assert(ecCtrl != nullptr);

    ecCtrl->SetValue(telegraphValue + GameParameters::EngineTelegraphDegreesOfFreedom / 2);
}

void SwitchboardPanel::OnEngineMonitorUpdated(
    ElectricalElementId electricalElementId,
    float /*thrustMagnitude*/,
    float rpm)
{
    LogMessage("SwitchboardPanel::OnEngineMonitorUpdated(", electricalElementId, "): RPM=", rpm);

    //
    // Toggle control
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Gauge);

    GaugeElectricalElementControl * ggCtrl = dynamic_cast<GaugeElectricalElementControl *>(elementInfo.Control);
    assert(ggCtrl != nullptr);

    ggCtrl->SetValue(1.0f - rpm);
}

///////////////////////////////////////////////////////////////////

void SwitchboardPanel::MakeSwitchPanel()
{
    // Create grid sizer for switch panel
    mSwitchPanelSizer = new wxGridBagSizer(0, 15);
    mSwitchPanelSizer->SetEmptyCellSize(mMinBitmapSize);

    // Create (scrollable) panel for switches
    mSwitchPanel = new wxScrolled<wxPanel>(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL);
    mSwitchPanel->SetScrollRate(5, 0);
    mSwitchPanel->FitInside();
    mSwitchPanel->SetSizerAndFit(mSwitchPanelSizer);
    mSwitchPanel->Bind(wxEVT_RIGHT_DOWN, &SwitchboardPanel::OnRightDown, this);

    // Add switch panel to v-sizer
    assert(mMainVSizer2->GetItemCount() == 1);
    mMainVSizer2->Add(mSwitchPanel, 0, wxALIGN_CENTER_HORIZONTAL); // We want it as wide and as tall as it is
}

void SwitchboardPanel::HideFully()
{
    // Hide hint panel
    mMainVSizer2->Hide(mHintPanel);
    ShowDockCheckbox(false);
    InstallMouseTracking(false);

    // Hide switch panel
    mMainVSizer2->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::NotShowing;
}

void SwitchboardPanel::ShowPartially()
{
    // Show hint panel
    InstallMouseTracking(true);
    ShowDockCheckbox(false);
    mMainVSizer2->Show(mHintPanel);

    // Hide switch panel
    mMainVSizer2->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingHint;
}

void SwitchboardPanel::ShowFullyFloating()
{
    // Show hint panel
    if (mDockCheckbox->IsChecked())
        mDockCheckbox->SetChecked(false);
    ShowDockCheckbox(true);
    InstallMouseTracking(true);
    mMainVSizer2->Show(mHintPanel);

    // Show switch panel
    mMainVSizer2->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyFloating;
}

void SwitchboardPanel::ShowFullyDocked()
{
    // Show hint panel
    if (!mDockCheckbox->IsChecked())
        mDockCheckbox->SetChecked(true);
    ShowDockCheckbox(true);
    InstallMouseTracking(false);
    mMainVSizer2->Show(mHintPanel);

    // Show switch panel
    mMainVSizer2->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyDocked;
}

void SwitchboardPanel::ShowDockCheckbox(bool doShow)
{
    assert(!!mMainHSizer1);
    assert(mMainHSizer1->GetItemCount() == 3);

    if (doShow)
    {
        if (!mMainHSizer1->IsShown(size_t(0)))
            mMainHSizer1->Show(size_t(0), true);
        if (!mMainHSizer1->IsShown(size_t(2)))
            mMainHSizer1->Show(size_t(2), true);
    }
    else
    {
        if (mMainHSizer1->IsShown(size_t(0)))
            mMainHSizer1->Show(size_t(0), false);
        if (mMainHSizer1->IsShown(size_t(2)))
            mMainHSizer1->Show(size_t(2), false);
    }
}

void SwitchboardPanel::InstallMouseTracking(bool isActive)
{
    assert(!!mLeaveWindowTimer);

    if (isActive && !mLeaveWindowTimer->IsRunning())
    {
        mLeaveWindowTimer->Start(750, false);
    }
    else if (!isActive && mLeaveWindowTimer->IsRunning())
    {
        mLeaveWindowTimer->Stop();
    }
}

void SwitchboardPanel::LayoutParent()
{
    mParentLayoutWindow->Layout();
}

void SwitchboardPanel::SetBackgroundBitmapFromCombo(int selection)
{
    assert(static_cast<unsigned int>(selection) < mBackgroundBitmapComboBox->GetCount());

    wxStringClientData * bitmapFilePath = dynamic_cast<wxStringClientData *>(mBackgroundBitmapComboBox->GetClientObject(selection));
    assert(nullptr != bitmapFilePath);

    wxBitmap backgroundBitmap;
    backgroundBitmap.LoadFile(bitmapFilePath->GetData(), wxBITMAP_TYPE_PNG);
    SetBackgroundBitmap(backgroundBitmap);

    Refresh();
}

void SwitchboardPanel::OnLeaveWindowTimer(wxTimerEvent & /*event*/)
{
    wxPoint const clientCoords = ScreenToClient(wxGetMousePosition());
    if (clientCoords.y < 0)
    {
        OnLeaveWindow();
    }
}

void SwitchboardPanel::OnDockCheckbox(wxCommandEvent & event)
{
    if (event.IsChecked())
    {
        //
        // Dock
        //

        ShowFullyDocked();

        // Re-layout from parent
        LayoutParent();

        // Play sound
        mSoundController->PlayElectricalPanelDockSound(false);
    }
    else
    {
        //
        // Undock
        //

        ShowFullyFloating();

        // Re-layout from parent
        LayoutParent();

        // Play sound
        mSoundController->PlayElectricalPanelDockSound(true);
    }
}

void SwitchboardPanel::OnEnterWindow(wxMouseEvent & /*event*/)
{
    if (mShowingMode == ShowingMode::ShowingHint)
    {
        //
        // Open the panel
        //

        ShowFullyFloating();

        // Re-layout from parent
        LayoutParent();

        // Play sound
        mSoundController->PlayElectricalPanelOpenSound(false);
    }
}

void SwitchboardPanel::OnLeaveWindow()
{
    if (mShowingMode == ShowingMode::ShowingFullyFloating)
    {
        //
        // Lower the panel
        //

        ShowPartially();

        // Re-layout from parent
        LayoutParent();

        // Play sound
        mSoundController->PlayElectricalPanelOpenSound(true);
    }
}

void SwitchboardPanel::OnRightDown(wxMouseEvent & event)
{
    assert(!!mBackgroundSelectorPopup);

    wxWindow * window = dynamic_cast<wxWindow *>(event.GetEventObject());
    if (nullptr == window)
        return;

    mBackgroundSelectorPopup->SetPosition(window->ClientToScreen(event.GetPosition()));
    mBackgroundSelectorPopup->Popup();
}

void SwitchboardPanel::OnBackgroundSelectionChanged(wxCommandEvent & /*event*/)
{
    int selection = mBackgroundBitmapComboBox->GetSelection();

    // Set bitmap
    SetBackgroundBitmapFromCombo(selection);

    // Remember preferences
    mUIPreferencesManager->SetSwitchboardBackgroundBitmapIndex(selection);
}