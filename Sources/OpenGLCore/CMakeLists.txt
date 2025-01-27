
#
# GameOpenGL library
#

set  (SOURCES
	GameOpenGL.cpp
	GameOpenGL.h
	GameOpenGL_Ext.cpp
	GameOpenGL_Ext.h
	GameOpenGLMappedBuffer.h
	ShaderManager.cpp.inl
	ShaderManager.h
	TriangleQuadElementArrayVBO.cpp
	TriangleQuadElementArrayVBO.h
	UploadedTextureManager.h
)

set  (GLAD_SOURCES
	glad/glad.c
	glad/glad.h
	glad/g_glad.h
	glad/khrplatform.h
)

source_group(" " FILES ${SOURCES})
source_group("glad" FILES ${GLAD_SOURCES})

add_library (GameOpenCore ${SOURCES} ${GLAD_SOURCES})

target_include_directories(GameOpenCore INTERFACE ..)

target_link_libraries (GameOpenCore
	Core
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

	target_link_libraries (GameOpenCore
		${CMAKE_DL_LIBS})

endif()