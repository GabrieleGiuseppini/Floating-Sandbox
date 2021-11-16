
#
# ShipBuilder library
#

set (SHIPBUILDER_LIB_SOURCES
	Controller.cpp
	Controller.h
	ElectricalElementInstanceIndexFactory.h
	IUserInterface.h
	MainFrame.cpp
	MainFrame.h
	MaterialPalette.cpp
	MaterialPalette.h
	Model.cpp
	Model.h
	ModelController.cpp
	ModelController.h
	ModelValidationDialog.cpp
	ModelValidationDialog.h
	ModelValidationResults.h
	NewPasswordDialog.cpp
	NewPasswordDialog.h
	SelectionManager.cpp	
	SelectionManager.h
	ShaderTypes.cpp
	ShaderTypes.h
	ShipBuilderTypes.h
	ShipCanvasResizeDialog.cpp
	ShipCanvasResizeDialog.h
	ShipPropertiesEditDialog.cpp
	ShipPropertiesEditDialog.h
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
	Tools/FloodTool.cpp
	Tools/FloodTool.h
	Tools/PencilTool.cpp
	Tools/PencilTool.h
	Tools/RopeEraserTool.cpp
	Tools/RopeEraserTool.h
	Tools/RopePencilTool.cpp
	Tools/RopePencilTool.h
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
