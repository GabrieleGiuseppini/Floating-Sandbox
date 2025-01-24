
#
# UI library
#

set  (UI_LIB_SOURCES
	BitmapButton.h
	BitmappedCheckbox.h
	BitmapPopupButton.h
	BitmapRadioButton.h
	BitmapToggleButton.h
	EditSpinBox.h
	ElectricalElementControl.cpp
	ElectricalElementControl.h
	HighlightableTextButton.h
	ImageLoadDialog.cpp
	ImageLoadDialog.h
	ImageLoader.cpp
	ImageLoader.h
	ImageSaveDialog.cpp
	ImageSaveDialog.h
	LayoutHelper.h
	LocalizationManager.cpp
	LocalizationManager.h
	LoggingDialog.cpp
	LoggingDialog.h
	ScalarTimeSeriesProbeControl.cpp
	ScalarTimeSeriesProbeControl.h
	ShipDescriptionDialog.cpp
	ShipDescriptionDialog.h
	ShipLoadDialog.cpp
	ShipLoadDialog.h
	ShipPreviewWindow.cpp
	ShipPreviewWindow.h
	ShipSaveDialog.cpp
	ShipSaveDialog.h
	SliderControl.h
	StandardSystemPaths.cpp
	StandardSystemPaths.h
	Style.h
	TextValidators.h
	UnderConstructionDialog.cpp
	UnderConstructionDialog.h
	UnFocusablePanel.h
	UnFocusableScrollablePanel.h
	WxHelpers.cpp
	WxHelpers.h
)

source_group(" " FILES ${UI_LIB_SOURCES})

add_library (UILib ${UI_LIB_SOURCES})

target_include_directories(UILib PRIVATE .)
target_include_directories(UILib PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(UILib PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (UILib
	GameCoreLib
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
