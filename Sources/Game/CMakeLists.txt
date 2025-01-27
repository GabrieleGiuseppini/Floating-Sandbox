
#
# Game library
#

set  (GAME_SOURCES	
	FileSystem.h
	GameAssetManager.cpp
	GameAssetManager.h
	PngImageFileTools.cpp
	PngImageFileTools.h
)

source_group(" " FILES ${GAME_SOURCES})

add_library (Game ${GAME_SOURCES})

target_include_directories(Game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Game PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Game INTERFACE ..)

target_link_libraries (Game
	Core
	${ADDITIONAL_LIBRARIES})
