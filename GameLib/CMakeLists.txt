
#
# Game library
#

set  (GAME_SOURCES
	Buffer.h
	CircularList.h
	ElementContainer.h
	EnumFlags.h
	FixedSizeVector.h
	FloatingPoint.h
	GameController.cpp
	GameController.h
	GameEventDispatcher.h
	GameException.h
	GameMath.h
	GameOpenGL.cpp
	GameOpenGL.h
	GameParameters.cpp
	GameParameters.h	
	GameRandomEngine.h
	GameTypes.cpp
	GameTypes.h
	GameWallClock.h
	IGameEventHandler.h
	ImageData.h
	ImageSize.h
	Log.cpp
	Log.h
	Material.cpp
	Material.h
	MaterialDatabase.h
	ObjectIdGenerator.h
	ProgressCallback.h
	RenderContext.cpp
	RenderContext.h	
	ResourceLoader.cpp
	ResourceLoader.h
	RotatedRectangle.h
	RotatedTextureRenderInfo.h
	ShipBuilder.cpp
	ShipBuilder.h
	ShipDefinition.h
	ShipDefinitionFile.cpp
	ShipDefinitionFile.h
	ShipRenderContext.cpp
	ShipRenderContext.h
	SysSpecifics.h
	TupleKeys.h
	Utils.cpp
	Utils.h	
	Vectors.cpp
	Vectors.h)

set  (GEOMETRY_SOURCES
	AABB.h
	Segment.h)

set  (PHYSICS_SOURCES
	Bomb.h
	Bombs.cpp
	Bombs.h
	Cloud.h
	ElectricalElements.cpp
	ElectricalElements.h
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
	TimerBomb.cpp
	TimerBomb.h
	Triangles.cpp
	Triangles.h
	WaterSurface.cpp
	WaterSurface.h
	World.cpp
	World.h)

source_group(" " FILES ${GAME_SOURCES})
source_group("Geometry" FILES ${GEOMETRY_SOURCES})
source_group("Physics" FILES ${PHYSICS_SOURCES})

add_library (GameLib ${GAME_SOURCES} ${GEOMETRY_SOURCES} ${PHYSICS_SOURCES})

target_include_directories(GameLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(GameLib PRIVATE ${IL_INCLUDE_DIR})
target_include_directories(GameLib INTERFACE ..)

target_link_libraries (GameLib
	GladLib
	${OPENGL_LIBRARIES}
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ILUT_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
