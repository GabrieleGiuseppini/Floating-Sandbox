
#
# Render library
#

set  (RENDER_SOURCES	
	GameShaderSet.cpp
	GameShaderSet.h
	GlobalRenderContext.cpp
	GlobalRenderContext.h
	RenderDeviceProperties.h
	RenderParameters.cpp
	RenderParameters.h
	ViewModel.h
)

source_group(" " FILES ${RENDER_SOURCES})

add_library (Render ${RENDER_SOURCES})

target_include_directories(Game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Game PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Game INTERFACE ..)

target_link_libraries (Render
	Core
	OpenGLCore
	${ADDITIONAL_LIBRARIES})
