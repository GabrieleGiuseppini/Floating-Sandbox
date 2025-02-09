
#
# Render library
#

set  (RENDER_SOURCES
	GameFontSet.h
	GameShaderSet.cpp
	GameShaderSet.h
	GameTextureDatabases.h
	GlobalRenderContext.cpp
	GlobalRenderContext.h
	NotificationRenderContext.cpp
	NotificationRenderContext.h
	RenderContext.cpp
	RenderContext.h
	RenderDeviceProperties.h
	RenderParameters.cpp
	RenderParameters.h
	RenderStatistics.h
	ShipRenderContext.cpp
	ShipRenderContext.h
	ViewModel.h
	WorldRenderContext.cpp
	WorldRenderContext.h
)

source_group(" " FILES ${RENDER_SOURCES})

add_library (Render ${RENDER_SOURCES})

target_include_directories(Render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Render PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Render INTERFACE ..)

target_link_libraries (Render
	Core
	OpenGLCore
	${ADDITIONAL_LIBRARIES})
