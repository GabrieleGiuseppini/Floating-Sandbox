set  (GLAD_SOURCES
	glad.c)

source_group(" " FILES ${GLAD_SOURCES})

add_library (GladLib ${GLAD_SOURCES})

target_include_directories(GladLib PRIVATE .)
target_include_directories(GladLib INTERFACE .)
