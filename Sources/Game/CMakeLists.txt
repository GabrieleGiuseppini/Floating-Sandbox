
#
# Game library
#

set  (GAME_SOURCES
	EnhancedShipPreviewData.h
	FileStreams.h
	FileSystem.h
	GameAssetManager.cpp
	GameAssetManager.h
	GameEventDispatcher.h
	GameVersion.h
	IGameEventHandlers.h
	RollingText.cpp
	RollingText.h
	Settings.cpp
	Settings.h
	ShipDeSerializer.cpp
	ShipDeSerializer.h
	ShipLegacyFormatDeSerializer.cpp
	ShipLegacyFormatDeSerializer.h
	ShipLoadSpecifications.h
)

source_group(" " FILES ${GAME_SOURCES})

add_library (Game ${GAME_SOURCES})

target_include_directories(Game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Game PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Game INTERFACE ..)

target_link_libraries (Game
	Core
	Render
	${ADDITIONAL_LIBRARIES})
