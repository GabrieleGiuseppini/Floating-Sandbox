set  (GPU_CALC_TEST_SOURCES
	AddTest.cpp
	AddTest.h
	MainApp.cpp
	MainFrame.cpp
	MainFrame.h
	OpenGLContext.cpp
	OpenGLContext.h
	OpenGLInitTest.h
	PixelCoordsTest.cpp
	PixelCoordsTest.h	
	TestCase.h
	TestRun.h)

source_group(" " FILES ${GPU_CALC_TEST_SOURCES})

add_executable (GPUCalcTest ${GPU_CALC_TEST_SOURCES})

target_include_directories(GPUCalcTest PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(GPUCalcTest PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (GPUCalcTest
	Core
	Game
	OpenGLCore
	GPUCalcLib
	${OPENGL_LIBRARIES}
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})


if (MSVC)
	set_target_properties(GPUCalcTest PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:MSVCRTD")
else (MSVC)
endif (MSVC)


#
# VS properties
#

if (MSVC)

	set_target_properties(
		GPUCalcTest
		PROPERTIES
			# Set debugger working directory to binary output directory
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

			# Set output directory to binary output directory - VS will add the configuration type
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)

endif (MSVC)



#
# Copy files
#

message (STATUS "Copying data files for GPUCalcTest...")

file(COPY "${CMAKE_SOURCE_DIR}/Data/Shaders/GPUCalc"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug/Data/Shaders")
file(COPY "${CMAKE_SOURCE_DIR}/Data/Shaders/GPUCalc"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release/Data/Shaders")
file(COPY "${CMAKE_SOURCE_DIR}/Data/Shaders/GPUCalc"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/Data/Shaders")



