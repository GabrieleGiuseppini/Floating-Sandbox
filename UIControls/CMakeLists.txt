
#
# UIControls library
#

set  (UI_CONTROLS_SOURCES
	ScalarTimeSeriesProbeControl.cpp
	ScalarTimeSeriesProbeControl.h
	ShipSwitchControl.h
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
