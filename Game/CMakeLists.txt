
#
# Game library
#

set  (GAME_SOURCES	
	ComputerCalibration.cpp
	ComputerCalibration.h
	ElectricalPanel.h
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
	LayerElements.h
	Layers.cpp
	Layers.h
	Materials.cpp
	Materials.h
	MaterialDatabase.cpp
	MaterialDatabase.h
	NotificationLayer.cpp
	NotificationLayer.h
	OceanFloorTerrain.cpp
	OceanFloorTerrain.h
	PerfStats.h
	RenderDeviceProperties.h
	ResourceLocator.cpp
	ResourceLocator.h
	RopeBuffer.h
	ShipAutoTexturizationSettings.cpp
	ShipAutoTexturizationSettings.h
	ShipDefinitionFormatDeSerializer.cpp
	ShipDefinitionFormatDeSerializer.h
	ShipDeSerializer.cpp
	ShipDeSerializer.h
	ShipFactory.cpp
	ShipFactory.h
	ShipFactoryTypes.h
	ShipDefinition.h
	ShipLegacyFormatDeSerializer.cpp
	ShipLegacyFormatDeSerializer.h
	ShipLoadOptions.h
	ShipLoadSpecifications.h
	ShipMetadata.h
	ShipPhysicsData.h
	ShipPreviewData.h
	ShipPreviewDirectoryManager.cpp
	ShipPreviewDirectoryManager.h
	ShipPreviewImageDatabase.cpp
	ShipPreviewImageDatabase.h
	ShipStrengthRandomizer.cpp
	ShipStrengthRandomizer.h
	ShipTexturizer.cpp
	ShipTexturizer.h
	ViewManager.cpp
	ViewManager.h
	VisibleWorld.h)

set  (PHYSICS_SOURCES
	AntiMatterBombGadget.h
	AntiMatterBombGadget.cpp
	Clouds.cpp
	Clouds.h	
	ElectricalElements.cpp
	ElectricalElements.h
	Fishes.cpp
	Fishes.h
	Formulae.h
	Frontiers.cpp
	Frontiers.h
	Gadget.h
	Gadgets.cpp
	Gadgets.h
	ImpactBombGadget.cpp
	ImpactBombGadget.h
	IShipPhysicsHandler.h
	Npcs.cpp
	Npcs.h
	NpcParticles.cpp
	NpcParticles.h
	OceanFloor.cpp
	OceanFloor.h
	OceanSurface.cpp
	OceanSurface.h
	Physics.h
	PhysicsProbeGadget.cpp
	PhysicsProbeGadget.h
	PhysicsTypes.h
	PinnedPoints.cpp
	PinnedPoints.h
	Points.cpp
	Points.h
	RCBombGadget.cpp
	RCBombGadget.h
	Ship.cpp
	Ship_ForceFields.cpp
	Ship_Interactions.cpp
	Ship_Interactions_Repair.cpp
	Ship_SpringRelaxation.cpp
	Ship_StateMachines.cpp
	Ship_StateMachines.h
	Ship.h
	ShipElectricSparks.cpp
	ShipElectricSparks.h
	ShipOverlays.cpp
	ShipOverlays.h
	Springs.cpp
	Springs.h
	Stars.cpp
	Stars.h
	Storm.cpp
	Storm.h
	TimerBombGadget.cpp
	TimerBombGadget.h
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
	TextureAtlas-inl.h
	TextureAtlas.h
	TextureDatabase-inl.h
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
	${ZLIB_LIBRARY}
	${JPEG_LIBRARY}
	${PNG_LIBRARY}
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ILUT_LIBRARIES}
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
