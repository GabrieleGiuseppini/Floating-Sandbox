set (SOURCES
	SoundAtlas.cpp
	SoundAtlas.h
	SoundParameters.h
	SoundTypes.h
)

source_group(" " FILES ${SOURCES})

add_library(SoundCore STATIC ${SOURCES})
set_target_properties(SoundCore PROPERTIES CXX_STANDARD 17)
target_include_directories(SoundCore PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(SoundCore INTERFACE ..)
target_link_libraries(SoundCore
	Core
	${ADDITIONAL_LIBRARIES}
)

