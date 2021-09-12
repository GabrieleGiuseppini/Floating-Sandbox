
#
# ShipBuilder library
#

set (SHIPBUILDER_LIB_SOURCES
	Controller.cpp
	Controller.h
	InputState.h
	IUserInterface.h
	MainFrame.cpp
	MainFrame.h
	MaterialPalette.cpp
	MaterialPalette.h
	Model.cpp
	Model.h
	ModelController.cpp
	ModelController.h
	ShaderTypes.cpp
	ShaderTypes.h
	ShipBuilderTypes.h
	View.h
	View.cpp
	ViewModel.h
	WorkbenchState.cpp
	WorkbenchState.h
)

set (TOOLS_SOURCES
	Tools/BaseTool.cpp
	Tools/BaseTool.h
	Tools/PencilTool.cpp
	Tools/PencilTool.h
	Tools/SelectionManager.cpp
	Tools/SelectionManager.h
)

source_group(" " FILES ${SHIPBUILDER_LIB_SOURCES})
source_group("Tools" FILES ${TOOLS_SOURCES})

add_library (ShipBuilderLib ${SHIPBUILDER_LIB_SOURCES} ${TOOLS_SOURCES})

target_include_directories(ShipBuilderLib PRIVATE .)
target_include_directories(ShipBuilderLib PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(ShipBuilderLib PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (ShipBuilderLib
	GameLib
	GameCoreLib
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
