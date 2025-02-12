
#
# GPUCalc library
#

set  (SOURCES
	AddGPUCalculator.cpp
	AddGPUCalculator.h
	GPUCalculator.cpp
	GPUCalculator.h
	GPUCalculatorFactory.cpp
	GPUCalculatorFactory.h
	IOpenGLContext.h
	PixelCoordsGPUCalculator.cpp
	PixelCoordsGPUCalculator.h
	ShaderSets.cpp
	ShaderSets.h
	)

source_group(" " FILES ${SOURCES})

add_library (GPUCalcLib ${SOURCES})

target_include_directories(GPUCalcLib INTERFACE ..)

target_link_libraries (GPUCalcLib
	Core
	OpenGLCore
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
