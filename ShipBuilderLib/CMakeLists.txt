
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
	Model.cpp
	Model.h
	ModelController.cpp
	ModelController.h
	ModelValidationResults.h
	OpenGLManager.h
	Preferences.cpp
	Preferences.h
	SelectionManager.cpp	
	SelectionManager.h
	ShaderTypes.cpp
	ShaderTypes.h
	ShipBuilderTypes.h
	UndoStack.cpp
	UndoStack.h
	View.cpp
	View.h
	ViewModel.h
	WorkbenchState.cpp
	WorkbenchState.h
)

set (UI_SOURCES
	AskPasswordDialog.cpp
	AskPasswordDialog.h
	MaterialPalette.cpp
	MaterialPalette.h
	ModelValidationDialog.cpp
	ModelValidationDialog.h
	NewPasswordDialog.cpp
	NewPasswordDialog.h
	ResizeDialog.cpp
	ResizeDialog.h
	RibbonToolbarButton.h
	ShipOffsetVisualizationControl.cpp
	ShipOffsetVisualizationControl.h
	ShipPropertiesEditDialog.cpp
	ShipPropertiesEditDialog.h
	ShipResizeVisualizationControl.cpp
	ShipResizeVisualizationControl.h
	StatusBar.cpp
	StatusBar.h
)

set (TOOLS_SOURCES
	Tools/FloodTool.cpp
	Tools/FloodTool.h
	Tools/LineTool.cpp
	Tools/LineTool.h
	Tools/PencilTool.cpp
	Tools/PencilTool.h
	Tools/RopeEraserTool.cpp
	Tools/RopeEraserTool.h
	Tools/RopePencilTool.cpp
	Tools/RopePencilTool.h
	Tools/SamplerTool.cpp
	Tools/SamplerTool.h
	Tools/Tool.cpp
	Tools/Tool.h
)

source_group(" " FILES ${SHIPBUILDER_LIB_SOURCES})
source_group("Tools" FILES ${TOOLS_SOURCES})
source_group("UI" FILES ${UI_SOURCES})

add_library (ShipBuilderLib ${SHIPBUILDER_LIB_SOURCES} ${TOOLS_SOURCES} ${UI_SOURCES})

target_include_directories(ShipBuilderLib PUBLIC .)
target_include_directories(ShipBuilderLib PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(ShipBuilderLib PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (ShipBuilderLib
	GameLib
	GameCoreLib
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
