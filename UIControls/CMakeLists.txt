
#
# UIControls library
#

set  (UI_CONTROLS_SOURCES
	BitmappedCheckbox.h
	ElectricalElementControl.h
	ScalarTimeSeriesProbeControl.cpp
	ScalarTimeSeriesProbeControl.h	
	SliderControl.h	
)

source_group(" " FILES ${UI_CONTROLS_SOURCES})

add_library (UIControlsLib ${UI_CONTROLS_SOURCES})

target_include_directories(UIControlsLib PRIVATE .)
target_include_directories(UIControlsLib PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(UIControlsLib PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (UIControlsLib
	GameCoreLib
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
