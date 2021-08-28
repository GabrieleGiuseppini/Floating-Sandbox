
#
# ShipBuilder library
#

set  (SHIPBUILDER_LIB_SOURCES
	MainFrame.h
	MainFrame.cpp
	ShipBuilderTypes.h
	View.h
	View.cpp
	ViewModel.h
)

source_group(" " FILES ${SHIPBUILDER_LIB_SOURCES})

add_library (ShipBuilderLib ${SHIPBUILDER_LIB_SOURCES})

target_include_directories(ShipBuilderLib PRIVATE .)
target_include_directories(ShipBuilderLib PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(ShipBuilderLib PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (ShipBuilderLib
	GameLib
	GameCoreLib
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
