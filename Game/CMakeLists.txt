
#
# Game library
#

set  (GAME_SOURCES	
	ComputerCalibration.cpp
	ComputerCalibration.h
	EventRecorder.h
	FishSpeciesDatabase.cpp
	FishSpeciesDatabase.h
	GameController.cpp
	GameController_StateMachines.cpp
	GameController.h
	GameEventDispatcher.h	
	GameParameters.cpp
	GameParameters.h
	IGameController.h
	IGameControllerSettings.h
	IGameControllerSettingsOptions.h
	IGameEventHandlers.h
	ImageFileTools.cpp
	ImageFileTools.h
	Materials.cpp
	Materials.h
	MaterialDatabase.h
	NotificationLayer.cpp
	NotificationLayer.h
	OceanFloorTerrain.cpp
	OceanFloorTerrain.h
	PerfStats.h
	ResourceLocator.cpp
	ResourceLocator.h
	ShipBuilder.cpp
	ShipBuilder.h
	ShipBuildTypes.h
	ShipDefinition.cpp
	ShipDefinition.h
	ShipDefinitionFile.cpp
	ShipDefinitionFile.h
	ShipMetadata.h
	ShipPreview.cpp
	ShipPreview.h
	ShipPreviewDirectoryManager.cpp
	ShipPreviewDirectoryManager.h
	ShipPreviewImageDatabase.cpp
	ShipPreviewImageDatabase.h
	ShipTexturizer.cpp
	ShipTexturizer.h
	VisibleWorld.h)

set  (PHYSICS_SOURCES
	AntiMatterBomb.h
	AntiMatterBomb.cpp
	Bomb.h
	Bombs.cpp
	Bombs.h
	Clouds.cpp
	Clouds.h
	ElectricalElements.cpp
	ElectricalElements.h
	Fishes.cpp
	Fishes.h
	Frontiers.cpp
	Frontiers.h
	ImpactBomb.cpp
	ImpactBomb.h
	IShipPhysicsHandler.h
	OceanFloor.cpp
	OceanFloor.h
	OceanSurface.cpp
	OceanSurface.h
	Physics.h
	PinnedPoints.cpp
	PinnedPoints.h
	Points.cpp
	Points.h
	RCBomb.cpp
	RCBomb.h
	Ship.cpp
	Ship_ForceFields.cpp
	Ship_Interactions.cpp
	Ship_StateMachines.cpp
	Ship_StateMachines.h
	Ship.h
	Springs.cpp
	Springs.h
	Stars.cpp
	Stars.h
	Storm.cpp
	Storm.h
	TimerBomb.cpp
	TimerBomb.h
	Triangles.cpp
	Triangles.h
	Wind.cpp
	Wind.h
	World.cpp
	World.h)

set  (RENDER_SOURCES
	Font.cpp
	Font.h
	GlobalRenderContext.cpp
	GlobalRenderContext.h
	NotificationRenderContext.cpp
	NotificationRenderContext.h
	RenderContext.cpp
	RenderContext.h
	RenderParameters.cpp
	RenderParameters.h
	RenderTypes.h
	ShaderTypes.cpp
	ShaderTypes.h
	ShipRenderContext.cpp
	ShipRenderContext.h
	TextureTypes.h
	TextureAtlas.cpp
	TextureAtlas.h
	TextureDatabase.cpp
	TextureDatabase.h
	UploadedTextureManager.h
	UploadedTextureManager-inl.h
	ViewModel.h
	WorldRenderContext.cpp
	WorldRenderContext.h)

source_group(" " FILES ${GAME_SOURCES})
source_group("Physics" FILES ${PHYSICS_SOURCES})
source_group("Render" FILES ${RENDER_SOURCES})

add_library (GameLib ${GAME_SOURCES} ${PHYSICS_SOURCES} ${RENDER_SOURCES})

target_include_directories(GameLib PRIVATE ${IL_INCLUDE_DIR})
target_include_directories(GameLib PUBLIC ${LIBSIMDPP_INCLUDE_DIRS})
target_include_directories(GameLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(GameLib INTERFACE ..)

target_link_libraries (GameLib
	GameCoreLib
	GameOpenGLLib
	GPUCalcLib
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ILUT_LIBRARIES}
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
