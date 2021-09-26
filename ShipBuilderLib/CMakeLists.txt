
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
	SaveShipDialog.cpp
	SaveShipDialog.h
	SelectionManager.cpp	
	SelectionManager.h
	ShaderTypes.cpp
	ShaderTypes.h
	ShipBuilderTypes.h
	StatusBar.cpp
	StatusBar.h
	UndoStack.cpp
	UndoStack.h
	View.cpp
	View.h
	ViewModel.h
	WorkbenchState.cpp
	WorkbenchState.h
)

set (TOOLS_SOURCES
	Tools/PencilTool.cpp
	Tools/PencilTool.h
	Tools/Tool.cpp
	Tools/Tool.h
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
