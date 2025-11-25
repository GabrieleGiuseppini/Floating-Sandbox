
#
# Game library
#

set  (GAME_SOURCES
	ComputerCalibration.cpp
	ComputerCalibration.h
	EnhancedShipPreviewData.h
	FileStreams.h
	FileSystem.h
	GameAssetManager.cpp
	GameAssetManager.h
	GameController.cpp
	GameController.h
	GameController_StateMachines.cpp
	GameEventDispatcher.h
	GameVersion.h
	IGameController.h
	IGameControllerSettings.h
	IGameControllerSettingsOptions.h
	IGameEventHandlers.h
	ISoundController.h
	NotificationLayer.cpp
	NotificationLayer.h
	RollingText.cpp
	RollingText.h
	Settings.cpp
	Settings.h
	ShipDeSerializer.cpp
	ShipDeSerializer.h
	ShipLegacyFormatDeSerializer.cpp
	ShipLegacyFormatDeSerializer.h
	ShipLoadSpecifications.h
	ShipPreviewDirectoryManager.cpp
	ShipPreviewDirectoryManager.h
	ShipPreviewImageDatabase.cpp
	ShipPreviewImageDatabase.h
	ViewManager.cpp
	ViewManager.h
)

source_group(" " FILES ${GAME_SOURCES})

add_library (Game ${GAME_SOURCES})

target_include_directories(Game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Game PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Game INTERFACE ..)

target_link_libraries (Game
	Core
	Render
	Simulation
	${ADDITIONAL_LIBRARIES})
