
#
# GameOpenGL library
#

set  (SOURCES
	GameOpenGL.cpp
	GameOpenGL.h
	GameOpenGL_Ext.cpp
	GameOpenGL_Ext.h
	ShaderManager.cpp.inl
	ShaderManager.h)

source_group(" " FILES ${SOURCES})

add_library (GameOpenGLLib ${SOURCES})

target_include_directories(GameOpenGLLib INTERFACE ..)

target_link_libraries (GameOpenGLLib
	GameCoreLib
	GladLib
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
