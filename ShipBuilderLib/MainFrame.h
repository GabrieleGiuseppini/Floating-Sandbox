/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"
#include "IUserInterface.h"
#include "MaterialPalette.h"
#include "ModelValidationDialog.h"
#include "OpenGLManager.h"
#include "ResizeDialog.h"
#include "RibbonToolbarButton.h"
#include "ShipPropertiesEditDialog.h"
#include "StatusBar.h"
#include "WorkbenchState.h"

#include <UILib/BitmapButton.h>
#include <UILib/BitmapRadioButton.h>
#include <UILib/BitmapToggleButton.h>
#include <UILib/LocalizationManager.h>
#include <UILib/LoggingDialog.h>
#include <UILib/ShipLoadDialog.h>
#include <UILib/ShipSaveDialog.h>

#include <Game/MaterialDatabase.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipTexturizer.h>

#include <wx/accel.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/glcanvas.h> // Need to include this *after* our glad.h has been included (from OpenGLManager.h)
#include <wx/icon.h>
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
#include <vector>

namespace ShipBuilder {

/*
 * The main window of the ship builder GUI.
 *
 * - Owns Controller
 * - Very thin, calls into Controller for each high-level interaction (e.g. new tool selected, tool setting changed) and for each mouse event
 * - Implements IUserInterface with interface needed by Controller, e.g. to make UI state changes, to capture the mouse, to update visualization of undo stack
 * - Owns WorkbenchState
 * - Implements ship load/save, giving/getting whole ShipDefinition to/from ModelController
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
        std::function<void(std::optional<std::filesystem::path>)> returnToGameFunctor);

    void OpenForNewShip();

    void OpenForLoadShip(std::filesystem::path const & shipFilePath);

public:

    //
    // IUserInterface
    //

    void RefreshView() override;

    void OnViewModelChanged(ViewModel const & viewModel) override;

    void OnShipSizeChanged(ShipSpaceSize const & shipSize) override;

    void OnShipNameChanged(Model const & model) override;

    void OnLayerPresenceChanged(Model const & model) override;

    void OnModelDirtyChanged(Model const & model) override;

    //

    void OnStructuralMaterialChanged(MaterialPlaneType plane, StructuralMaterial const * material) override;
    void OnElectricalMaterialChanged(MaterialPlaneType plane, ElectricalMaterial const * material) override;
    void OnRopesMaterialChanged(MaterialPlaneType plane, StructuralMaterial const * material) override;

    void OnCurrentToolChanged(std::optional<ToolType> tool) override;

    void OnPrimaryVisualizationChanged(VisualizationType primaryVisualization) override;

    void OnGameVisualizationModeChanged(GameVisualizationModeType mode) override;
    void OnStructuralLayerVisualizationModeChanged(StructuralLayerVisualizationModeType mode) override;
    void OnElectricalLayerVisualizationModeChanged(ElectricalLayerVisualizationModeType mode) override;
    void OnRopesLayerVisualizationModeChanged(RopesLayerVisualizationModeType mode) override;
    void OnTextureLayerVisualizationModeChanged(TextureLayerVisualizationModeType mode) override;

    void OnOtherVisualizationsOpacityChanged(float opacity) override;

    void OnVisualGridEnablementChanged(bool isEnabled) override;

    //

    void OnUndoStackStateChanged(UndoStack & undoStack) override;

    void OnToolCoordinatesChanged(std::optional<ShipSpaceCoordinates> coordinates) override;

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
    wxRibbonPage * CreateLayersRibbonPage(wxRibbonBar * parent);
    wxRibbonPanel * CreateLayerRibbonPanel(wxRibbonPage * parent, LayerType layer);
    wxRibbonPage * CreateEditRibbonPage(wxRibbonBar * parent);
    wxRibbonPanel * CreateEditUndoRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateEditShipRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateEditAnalysisRibbonPanel(wxRibbonPage * parent);
    wxRibbonPanel * CreateEditToolSettingsRibbonPanel(wxRibbonPage * parent);
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
    void OnStructuralMaterialSelected(fsStructuralMaterialSelectedEvent & event);
    void OnElectricalMaterialSelected(fsElectricalMaterialSelectedEvent & event);
    void OnRopeMaterialSelected(fsStructuralMaterialSelectedEvent & event);

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

    void SaveAndSwitchBackToGame();

    void QuitAndSwitchBackToGame();

    void Quit();

    void SwitchBackToGame(std::optional<std::filesystem::path> shipFilePath);

    void ImportTextureLayerFromImage();

    void OpenShipCanvasResize();

    void OpenShipProperties();

    void ValidateShip();

    void OpenMaterialPalette(
        wxMouseEvent const & event,
        LayerType layer,
        MaterialPlaneType plane);

    bool AskUserIfSure(wxString caption);

    int AskUserIfSave();

    bool AskUserIfRename(std::string const & newFilename);

    void ShowError(wxString const & message);

    void DoNewShip();

    bool DoLoadShip(std::filesystem::path const & shipFilePath);

    bool DoSaveShipOrSaveShipAsWithValidation();

    bool DoSaveShipAsWithValidation();

    bool DoSaveShipWithValidation(std::filesystem::path const & shipFilePath);

    void DoSaveShipWithoutValidation(std::filesystem::path const & shipFilePath);

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

    void ReconciliateUIWithShipTitle(std::string const & shipName, bool isShipDirty);

    void ReconciliateUIWithLayerPresence(Model const & model);
    
    void ReconciliateUIWithModelDirtiness(Model const & model);

    //

    void ReconciliateUIWithStructuralMaterial(MaterialPlaneType plane, StructuralMaterial const * material);
    void ReconciliateUIWithElectricalMaterial(MaterialPlaneType plane, ElectricalMaterial const * material);
    void ReconciliateUIWithRopesMaterial(MaterialPlaneType plane, StructuralMaterial const * material);

    void ReconciliateUIWithSelectedTool(std::optional<ToolType> tool);

    void ReconciliateUIWithPrimaryVisualizationSelection(VisualizationType primaryVisualization);

    void ReconciliateUIWithGameVisualizationModeSelection(GameVisualizationModeType mode);
    void ReconciliateUIWithStructuralLayerVisualizationModeSelection(StructuralLayerVisualizationModeType mode);
    void ReconciliateUIWithElectricalLayerVisualizationModeSelection(ElectricalLayerVisualizationModeType mode);
    void ReconciliateUIWithRopesLayerVisualizationModeSelection(RopesLayerVisualizationModeType mode);
    void ReconciliateUIWithTextureLayerVisualizationModeSelection(TextureLayerVisualizationModeType mode);

    void ReconciliateUIWithOtherVisualizationsOpacity(float opacity);

    void ReconciliateUIWithVisualGridEnablement(bool isEnabled);

    //

    void ReconciliateUIWithUndoStackState(UndoStack & undoStack);

private:

    wxApp * const mMainApp;

    std::function<void(std::optional<std::filesystem::path>)> const mReturnToGameFunctor;

    //
    // Owned members
    //

    std::unique_ptr<OpenGLManager> mOpenGLManager;

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
    RibbonToolbarButton<BitmapButton> * mSaveShipAsButton;
    RibbonToolbarButton<BitmapButton> * mSaveShipAndGoBackButton;
    RibbonToolbarButton<BitmapButton> * mZoomInButton;
    RibbonToolbarButton<BitmapButton> * mZoomOutButton;
    RibbonToolbarButton<BitmapButton> * mUndoButton;
    std::array<RibbonToolbarButton<BitmapRadioButton> *, VisualizationCount> mVisualizationSelectButtons;
    std::array<RibbonToolbarButton<BitmapButton> *, LayerCount> mLayerExportButtons;
    std::array<RibbonToolbarButton<BitmapButton> *, LayerCount> mLayerDeleteButtons;
    wxSizer * mToolSettingsPanelsSizer;
    std::vector<std::tuple<ToolType, wxPanel *>> mToolSettingsPanels;

    // Visualization details panel
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
    wxBitmapToggleButton * mViewGridButton;

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
    std::unique_ptr<wxGLCanvas> mWorkCanvas;
    wxScrollBar * mWorkCanvasHScrollBar;
    wxScrollBar * mWorkCanvasVScrollBar;

    // Misc UI elements
    std::unique_ptr<MaterialPalette<LayerType::Structural>> mStructuralMaterialPalette;
    std::unique_ptr<MaterialPalette<LayerType::Electrical>> mElectricalMaterialPalette;
    std::unique_ptr<MaterialPalette<LayerType::Ropes>> mRopesMaterialPalette;
    StatusBar * mStatusBar;

    //
    // Dialogs
    //

    std::unique_ptr<ShipLoadDialog> mShipLoadDialog;
    std::unique_ptr<ShipSaveDialog> mShipSaveDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;
    std::unique_ptr<ResizeDialog> mResizeDialog;
    std::unique_ptr<ShipPropertiesEditDialog> mShipPropertiesEditDialog;
    std::unique_ptr<ModelValidationDialog> mModelValidationDialog;

    //
    // UI state
    //

    bool mutable mIsMouseInWorkCanvas;
    bool mutable mIsMouseCapturedByWorkCanvas;

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
    std::vector<std::filesystem::path> mShipLoadDirectories;
};

}