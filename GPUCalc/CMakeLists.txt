
#
# GPUCalc library
#

set  (SOURCES
	GPUCalcContext.cpp
	GPUCalcContext.h
	GPUCalcContextFactory.cpp
	GPUCalcContextFactory.h
	IOpenGLContext.h
	TestGPUCalcContext.cpp
	TestGPUCalcContext.h)

source_group(" " FILES ${SOURCES})

add_library (GPUCalcLib ${SOURCES})

target_include_directories(GPUCalcLib INTERFACE ..)

target_link_libraries (GPUCalcLib
	GameCoreLib
	GameOpenGLLib
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
