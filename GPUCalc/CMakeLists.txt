
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
	ShaderTraits.cpp
	ShaderTraits.h
	)

source_group(" " FILES ${SOURCES})

add_library (GPUCalcLib ${SOURCES})

target_include_directories(GPUCalcLib INTERFACE ..)

target_link_libraries (GPUCalcLib
	GameCoreLib
	GameOpenGLLib
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
