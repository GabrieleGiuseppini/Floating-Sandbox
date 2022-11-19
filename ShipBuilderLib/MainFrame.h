/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"
#include "IUserInterface.h"
#include "OpenGLManager.h"
#include "ShipNameNormalizer.h"
#include "WorkbenchState.h"

#include "UI/CompositeMaterialPalette.h"
#include "UI/ElectricalPanelEditDialog.h"
#include "UI/ModelValidationDialog.h"
#include "UI/PreferencesDialog.h"
#include "UI/ResizeDialog.h"
#include "UI/RibbonToolbarButton.h"
#include "UI/ShipPropertiesEditDialog.h"
#include "UI/StatusBar.h"

#include <UILib/BitmapButton.h>
#include <UILib/BitmapRadioButton.h>
#include <UILib/EditSpinBox.h>
#include <UILib/LocalizationManager.h>
#include <UILib/LoggingDialog.h>
#include <UILib/ShipLoadDialog.h>
#include <UILib/ShipSaveDialog.h>

#include <Game/MaterialDatabase.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipTexturizer.h>

#include <GameCore/GameTypes.h>
#include <GameCore/ProgressCallback.h>

#include <wx/accel.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/glcanvas.h> // Need to include this *after* our glad.h has been included (from OpenGLManager.h)
#include <wx/icon.h>
#include <wx/notifmsg.h>
#include <wx/panel.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/page.h>
#include <wx/scrolbar.h>
#include <wx/scrolwin.h>
#include <wx/slider.h>
#include <wx/statbmp.h>

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * The main window of the ship builder GUI.
 *
 * - Owns Controller
 * - Very thin, calls into Controller for each high-level interaction (e.g. new tool selected, tool setting changed) and for each mouse event
 * - Implements IUserInterface with interface needed by Controller, e.g. to make UI state changes, to capture the mouse, to update visualization of undo stack
 * - Owns WorkbenchState
 * - Implements ship load/save, giving/getting whole ShipDefinition to/from Controller
 */
class MainFrame final : public wxFrame, public IUserInterface
{
public:

    MainFrame(
        wxApp * mainApp,
        wxIcon const & icon,
        ResourceLocator const & resourceLocator,
        LocalizationManager const & localizationManager,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        std::function<void(std::optional<std::filesystem::path>)> returnToGameFunctor,
        ProgressCallback const & progressCallback);

    ~MainFrame();

    void OpenForNewShip(std::optional<UnitsSystem> displayUnitsSystem);

    void OpenForLoadShip(
        std::filesystem::path const & shipFilePath,
        std::optional<UnitsSystem> displayUnitsSystem);

public:

    //
    // IUserInterface
    //

    void RefreshView() override;

    void OnViewModelChanged(ViewModel const & viewModel) override;

    void OnShipSizeChanged(ShipSpaceSize const & shipSize) override;

    void OnShipScaleChanged(ShipSpaceToWorldSpaceCoordsRatio const & scale) override;

    void OnShipNameChanged(IModelObservable const & model) override;

    void OnLayerPresenceChanged(IModelObservable const & model) override;

    void OnModelDirtyChanged(IModelObservable const & model) override;

    void OnModelMacroPropertiesUpdated(ModelMacroProperties const & properties) override;

    void OnElectricalLayerInstancedElementSetChanged(InstancedElectricalElementSet const & instancedElectricalElementSet) override;

    //

    void OnStructuralMaterialChanged(StructuralMaterial const * material, MaterialPlaneType plane) override;
    void OnElectricalMaterialChanged(ElectricalMaterial const * material, MaterialPlaneType plane) override;
    void OnRopesMaterialChanged(StructuralMaterial const * material, MaterialPlaneType plane) override;

    void OnCurrentToolChanged(ToolType tool, bool isFromUser) override;

    void OnPrimaryVisualizationChanged(VisualizationType primaryVisualization) override;

    void OnGameVisualizationModeChanged(GameVisualizationModeType mode) override;
    void OnStructuralLayerVisualizationModeChanged(StructuralLayerVisualizationModeType mode) override;
    void OnElectricalLayerVisualizationModeChanged(ElectricalLayerVisualizationModeType mode) override;
    void OnRopesLayerVisualizationModeChanged(RopesLayerVisualizationModeType mode) override;
    void OnTextureLayerVisualizationModeChanged(TextureLayerVisualizationModeType mode) override;

    void OnOtherVisualizationsOpacityChanged(float opacity) override;

    void OnVisualWaterlineMarkersEnablementChanged(bool isEnabled) override;
    void OnVisualGridEnablementChanged(bool isEnabled) override;

    //

    void OnUndoStackStateChanged(UndoStack & undoStack) override;

    void OnSelectionChanged(std::optional<ShipSpaceRect> const & selectionRect) override;

    void OnClipboardChanged(bool isPopulated) override;

    void OnToolCoordinatesChanged(std::optional<ShipSpaceCoordinates> coordinates, ShipSpaceSize const & shipSize) override;

    void OnSampledInformationUpdated(std::optional<SampledInformation> sampledInformation) override;

    void OnMeasuredWorldLengthChanged(std::optional<int> length) override;

    void OnMeasuredSelectionSizeChanged(std::optional<ShipSpaceSize> selectionSize) override;

    void OnError(wxString const & errorMessage) const override;

    DisplayLogicalSize GetDisplaySize() const override;

    int GetLogicalToPhysicalPixelFactor() const override;

    void SwapRenderBuffers() override;

    DisplayLogicalCoordinates GetMouseCoordinates() const override;

    bool IsMouseInWorkCanvas() const override;

    std::optional<DisplayLogicalCoordinates> GetMouseCoordinatesIfInWorkCanvas() const override;

    void SetToolCursor(wxImage const & cursorImage) override;

    void ResetToolCursor() override;

private:

    wxRibbonPage * CreateMainRibbonPage(wxRibbonBar * parent);
    wxRibbonPanel * CreateMainFileRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateMainViewRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateMainPreferencesRibbonPanel(wxRibbonPage * parent);
    wxRibbonPage * CreateLayersRibbonPage(wxRibbonBar * parent);
    wxRibbonPanel * CreateLayerRibbonPanel(wxRibbonPage * parent, LayerType layer);
    wxRibbonPage * CreateEditRibbonPage(wxRibbonBar * parent);
    wxRibbonPanel * CreateEditUndoRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateEditShipRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateEditEditRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateEditAnalysisRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateEditToolSettingsRibbonPanel(wxRibbonPage * parent);
    wxPanel * CreateVisualizationModeHeaderPanel(wxWindow * parent);
    wxPanel * CreateVisualizationDetailsPanel(wxWindow * parent);
    wxPanel * CreateToolbarPanel(wxWindow * parent);
    wxPanel * CreateUndoPanel(wxWindow * parent);
    wxPanel * CreateWorkPanel(wxWindow * parent);
    void AddAcceleratorKey(int flags, int keyCode, std::function<void()> handler);

    void OnWorkCanvasPaint(wxPaintEvent & event);
    void OnWorkCanvasResize(wxSizeEvent & event);
    void OnWorkCanvasLeftDown(wxMouseEvent & event);
    void OnWorkCanvasLeftUp(wxMouseEvent & event);
    void OnWorkCanvasRightDown(wxMouseEvent & event);
    void OnWorkCanvasRightUp(wxMouseEvent & event);
    void OnWorkCanvasMouseMove(wxMouseEvent & event);
    void OnWorkCanvasMouseWheel(wxMouseEvent & event);
    void OnWorkCanvasCaptureMouseLost(wxMouseCaptureLostEvent & event);
    void OnWorkCanvasMouseLeftWindow(wxMouseEvent & event);
    void OnWorkCanvasMouseEnteredWindow(wxMouseEvent & event);
    void OnWorkCanvasKeyDown(wxKeyEvent & event);
    void OnWorkCanvasKeyUp(wxKeyEvent & event);

    void OnClose(wxCloseEvent & event);

private:

    bool IsStandAlone() const
    {
        return !mReturnToGameFunctor;
    }

    void Open();

    void NewShip();

    void LoadShip();

    void SaveShip();

    void SaveShipAs();

    void BackupShip();

    void SaveAndSwitchBackToGame();

    void QuitAndSwitchBackToGame();

    void Quit();

    void SwitchBackToGame(std::optional<std::filesystem::path> shipFilePath);

    void ImportLayerFromShip(LayerType layer);

    void ImportTextureLayerFromImage();

    void OnPreferences();

    void OnShipCanvasResize();

    void OnShipPropertiesEdit();

    void OnElectricalPanelEdit();

    void Copy();

    void Cut();

    void Paste();

    void ValidateShip();

    void SelectAll();

    void Deselect();

    void PasteRotate90CW();
    void PasteRotate90CCW();
    void PasteFlipH();
    void PasteFlipV();
    void PasteCommit();
    void PasteAbort();

    void OpenMaterialPalette(
        wxMouseEvent const & event,
        LayerType layer,
        MaterialPlaneType plane);

    bool AskUserIfSure(wxString caption);

    int AskUserIfSave();

    bool AskUserIfRename(std::string const & newFilename);

    void ShowError(wxString const & message) const;

    void ShowNotification(wxString const & message) const;

    void DoNewShip();

    bool DoLoadShip(std::filesystem::path const & shipFilePath);

    std::optional<ShipDefinition> DoLoadShipDefinitionAndCheckPassword(std::filesystem::path const & shipFilePath);

    bool DoSaveShipOrSaveShipAsWithValidation();

    bool DoSaveShipAsWithValidation();

    bool DoSaveShipWithValidation(std::filesystem::path const & shipFilePath);

    void DoSaveShipWithoutValidation(std::filesystem::path const & shipFilePath);

    static void DoSaveShipDefinition(Controller const & controller, std::filesystem::path const & shipFilePath);

    bool DoPreSaveShipValidation();

    void BailOut();

    bool IsLogicallyInWorkCanvas(DisplayLogicalCoordinates const & coords) const;

    DisplayLogicalSize GetWorkCanvasSize() const;

    void ZoomIn();

    void ZoomOut();

    void ResetView();

    void RecalculateWorkCanvasPanning(ViewModel const & viewModel);

    void SetFrameTitle(std::string const & shipName, bool isDirty);

    void DeviateFocus();

    float OtherVisualizationsOpacitySliderToOpacity(int sliderValue) const;

    int OtherVisualizationsOpacityToSlider(float opacityValue) const;

    size_t LayerToVisualizationIndex(LayerType layer) const;

    //
    // UI Consistency
    //

    void ReconciliateUIWithWorkbenchState();

    void ReconciliateUIWithViewModel(ViewModel const & viewModel);

    void ReconciliateUIWithShipSize(ShipSpaceSize const & shipSize);

    void ReconciliateUIWithShipScale(ShipSpaceToWorldSpaceCoordsRatio const & scale);

    void ReconciliateUIWithShipTitle(std::string const & shipName, bool isShipDirty);

    void ReconciliateUIWithLayerPresence(IModelObservable const & model);

    void ReconciliateUIWithModelDirtiness(IModelObservable const & model);

    void ReconciliateUIWithModelMacroProperties(ModelMacroProperties const & properties);

    void ReconciliateUIWithElectricalLayerInstancedElementSet(InstancedElectricalElementSet const & instancedElectricalElementSet);

    //

    void ReconciliateUIWithStructuralMaterial(StructuralMaterial const * material, MaterialPlaneType plane);
    void ReconciliateUIWithElectricalMaterial(ElectricalMaterial const * material, MaterialPlaneType plane);
    void ReconciliateUIWithRopesMaterial(StructuralMaterial const * material, MaterialPlaneType plane);

    void ReconciliateUIWithSelectedTool(
        ToolType tool,
        bool isFromUser);

    void ReconciliateUIWithPrimaryVisualizationSelection(VisualizationType primaryVisualization);

    void ReconciliateUIWithGameVisualizationModeSelection(GameVisualizationModeType mode);
    void ReconciliateUIWithStructuralLayerVisualizationModeSelection(StructuralLayerVisualizationModeType mode);
    void ReconciliateUIWithElectricalLayerVisualizationModeSelection(ElectricalLayerVisualizationModeType mode);
    void ReconciliateUIWithRopesLayerVisualizationModeSelection(RopesLayerVisualizationModeType mode);
    void ReconciliateUIWithTextureLayerVisualizationModeSelection(TextureLayerVisualizationModeType mode);

    void ReconciliateUIWithOtherVisualizationsOpacity(float opacity);

    void ReconciliateUIWithVisualWaterlineMarkersEnablement(bool isEnabled);
    void ReconciliateUIWithVisualGridEnablement(bool isEnabled);

    //

    void ReconciliateUIWithUndoStackState(UndoStack & undoStack);

    void ReconciliateUIWithSelection(std::optional<ShipSpaceRect> const & selectionRect);
    void ReconciliateUIWithClipboard(bool isPopulated);

    void ReconciliateUIWithDisplayUnitsSystem(UnitsSystem displayUnitsSystem);
    void ReconciliateUIWithShipFilename();

private:

    wxApp * const mMainApp;

    std::function<void(std::optional<std::filesystem::path>)> const mReturnToGameFunctor;

    //
    // Owned members
    //

    std::unique_ptr<OpenGLManager> mOpenGLManager;
    std::unique_ptr<ShipNameNormalizer> mShipNameNormalizer;

    std::unique_ptr<Controller> mController; // Comes and goes as we are opened/close

    //
    // Helpers
    //

    ResourceLocator const & mResourceLocator;
    LocalizationManager const & mLocalizationManager;
    MaterialDatabase const & mMaterialDatabase;
    ShipTexturizer const & mShipTexturizer;

    //
    // UI
    //

    wxPanel * mMainPanel;
    std::vector<wxAcceleratorEntry> mAcceleratorEntries;

    // Ribbon bar
    wxRibbonBar * mMainRibbonBar;
    RibbonToolbarButton<BitmapButton> * mSaveShipButton;
    RibbonToolbarButton<BitmapButton> * mBackupShipButton;
    RibbonToolbarButton<BitmapButton> * mSaveShipAndGoBackButton;
    RibbonToolbarButton<BitmapButton> * mZoomInButton;
    RibbonToolbarButton<BitmapButton> * mZoomOutButton;
    RibbonToolbarButton<BitmapButton> * mUndoButton;
    RibbonToolbarButton<BitmapButton> * mCopyButton;
    RibbonToolbarButton<BitmapButton> * mCutButton;
    RibbonToolbarButton<BitmapButton> * mPasteButton;
    std::array<RibbonToolbarButton<BitmapRadioButton> *, VisualizationCount> mVisualizationSelectButtons;
    std::array<RibbonToolbarButton<BitmapButton> *, LayerCount> mLayerExportButtons;
    std::array<RibbonToolbarButton<BitmapButton> *, LayerCount> mLayerDeleteButtons;
    RibbonToolbarButton<BitmapButton> * mElectricalPanelEditButton;
    wxRibbonPanel * mToolSettingsRibbonPanel;
    wxSizer * mToolSettingsPanelsSizer;
    std::vector<std::tuple<std::vector<ToolType>, wxPanel *>> mToolSettingsPanels;
    wxSlider * mTextureMagicWandToleranceSlider;
    EditSpinBox<std::uint32_t> * mTextureMagicWandToleranceEditSpinBox;
    BitmapButton * mDeselectButton;


    // Visualization details panel
    wxSizer * mVisualizationModeHeaderPanelsSizer;
    std::array<wxPanel *, VisualizationCount> mVisualizationModeHeaderPanels;
    wxSlider * mOtherVisualizationsOpacitySlider;
    wxSizer * mVisualizationModePanelsSizer;
    std::array<wxPanel *, VisualizationCount> mVisualizationModePanels;
    BitmapRadioButton * mGameVisualizationNoneModeButton;
    BitmapRadioButton * mGameVisualizationAutoTexturizationModeButton;
    BitmapRadioButton * mGameVisualizationTextureModeButton;
    BitmapRadioButton * mStructuralLayerVisualizationNoneModeButton;
    BitmapRadioButton * mStructuralLayerVisualizationMeshModeButton;
    BitmapRadioButton * mStructuralLayerVisualizationPixelModeButton;
    BitmapRadioButton * mElectricalLayerVisualizationNoneModeButton;
    BitmapRadioButton * mElectricalLayerVisualizationPixelModeButton;
    BitmapRadioButton * mRopesLayerVisualizationNoneModeButton;
    BitmapRadioButton * mRopesLayerVisualizationLinesModeButton;
    BitmapRadioButton * mTextureLayerVisualizationNoneModeButton;
    BitmapRadioButton * mTextureLayerVisualizationMatteModeButton;
    BitmapToggleButton * mViewWaterlineMarkersButton;
    BitmapToggleButton * mViewGridButton;

    // Toolbar panel
    wxSizer * mToolbarPanelsSizer;
    std::array<wxPanel *, LayerCount> mToolbarPanels;
    std::array<BitmapRadioButton *, static_cast<size_t>(ToolType::_Last) + 1> mToolButtons;
    wxStaticBitmap * mStructuralForegroundMaterialSelector;
    wxStaticBitmap * mStructuralBackgroundMaterialSelector;
    wxStaticBitmap * mElectricalForegroundMaterialSelector;
    wxStaticBitmap * mElectricalBackgroundMaterialSelector;
    wxStaticBitmap * mRopesForegroundMaterialSelector;
    wxStaticBitmap * mRopesBackgroundMaterialSelector;
    wxBitmap mNullMaterialBitmap;

    // Undo stack panel
    wxScrolledWindow * mUndoStackPanel;

    // Work panel
    wxGLCanvas * mWorkCanvas;
    wxScrollBar * mWorkCanvasHScrollBar;
    wxScrollBar * mWorkCanvasVScrollBar;

    // Misc UI elements
    std::unique_ptr<CompositeMaterialPalette> mCompositeMaterialPalette;
    StatusBar * mStatusBar;

    //
    // Dialogs
    //

    std::unique_ptr<wxNotificationMessage> mNotificationMessage;
    std::unique_ptr<ShipLoadDialog<ShipLoadDialogUsageType::ForShipBuilder>> mShipLoadDialog;
    std::unique_ptr<ShipSaveDialog> mShipSaveDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;
    std::unique_ptr<PreferencesDialog> mPreferencesDialog;
    std::unique_ptr<ResizeDialog> mResizeDialog;
    std::unique_ptr<ShipPropertiesEditDialog> mShipPropertiesEditDialog;
    std::unique_ptr<ModelValidationDialog> mModelValidationDialog;
    std::unique_ptr<ElectricalPanelEditDialog> mElectricalPanelEditDialog;

    //
    // UI state
    //

    bool mutable mIsMouseInWorkCanvas;
    bool mutable mIsMouseCapturedByWorkCanvas;
    bool mutable mIsShiftKeyDown;

    //
    // Open action
    //
    // This is the mechanism that allows the builder to initialize on the first OnPaint event
    //

    using InitialAction = std::function<void()>;

    // When set, we still have to perform the initial action
    std::optional<InitialAction> mInitialAction;

    //
    // State
    //

    WorkbenchState mWorkbenchState;

    std::optional<std::filesystem::path> mCurrentShipFilePath;
};

}