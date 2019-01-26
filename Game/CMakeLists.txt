
#
# Game library
#

set  (GAME_SOURCES
	GameController.cpp
	GameController.h
	GameEventDispatcher.h
	GameParameters.cpp
	GameParameters.h
	IGameEventHandler.h
	ImageFileTools.cpp
	ImageFileTools.h
	Materials.cpp
	Materials.h
	MaterialDatabase.h
	ResourceLoader.cpp
	ResourceLoader.h
	ShipBuilder.cpp
	ShipBuilder.h
	ShipDefinition.cpp
	ShipDefinition.h
	ShipDefinitionFile.cpp
	ShipDefinitionFile.h
	ShipMetadata.h
	ShipPreview.cpp
	ShipPreview.h
	TextLayer.cpp
	TextLayer.h)

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
	ForceFields.cpp
	ForceFields.h
	ImpactBomb.cpp
	ImpactBomb.h
	OceanFloor.cpp
	OceanFloor.h
	Physics.h
	PinnedPoints.cpp
	PinnedPoints.h
	Points.cpp
	Points.h
	RCBomb.cpp
	RCBomb.h
	Ship.cpp
	Ship.h
	Springs.cpp
	Springs.h
	Stars.cpp
	Stars.h
	TimerBomb.cpp
	TimerBomb.h
	Triangles.cpp
	Triangles.h
	WaterSurface.cpp
	WaterSurface.h
	Wind.cpp
	Wind.h
	World.cpp
	World.h)

set  (RENDER_SOURCES
	Font.cpp
	Font.h
	RenderContext.cpp
	RenderContext.h
	RenderCore.cpp
	RenderCore.h
	ShipRenderContext.cpp
	ShipRenderContext.h
	TextRenderContext.cpp
	TextRenderContext.h
	TextureAtlas.cpp
	TextureAtlas.h
	TextureDatabase.cpp
	TextureDatabase.h
	TextureRenderManager.cpp
	TextureRenderManager.h)

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
