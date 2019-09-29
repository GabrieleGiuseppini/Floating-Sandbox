enable_testing()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING /DGTEST_HAS_TR1_TUPLE=0")
endif(MSVC)

#
# Setup google test
#

set_property(
	TARGET gmock gtest gtest_main
	PROPERTY FOLDER "GoogleTest")

#
# Setup target
#

set (UNIT_TEST_SOURCES
	AlgorithmsTests.cpp
	BoundedVectorTests.cpp
	BufferTests.cpp
	CircularListTests.cpp
	EnumFlagsTests.cpp
	FixedSizeVectorTests.cpp
	GameEventDispatcherTests.cpp
	GameMathTests.cpp
	ParameterSmootherTests.cpp
	PrecalculatedFunctionTests.cpp
	SegmentTests.cpp
	ShaderManagerTests.cpp
	SliderCoreTests.cpp
	SysSpecificsTests.cpp
	TemporallyCoherentPriorityQueueTests.cpp
	TextureAtlasTests.cpp
	TruncatedPriorityQueueTests.cpp
	TupleKeysTests.cpp
	Utils.cpp
	Utils.h
	VectorsTests.cpp
	VersionTests.cpp
)

source_group(" " FILES ${UNIT_TEST_SOURCES})

add_executable (UnitTests ${UNIT_TEST_SOURCES})
add_test (UnitTests UnitTests)

target_include_directories(UnitTests PRIVATE SYSTEM ${LIBSIMDPP_INCLUDE_DIRS})

target_link_libraries (UnitTests
	GameCoreLib
	GameLib
	GPUCalcLib
	${OPENGL_LIBRARIES}
	gmock
	gtest_main
	${ADDITIONAL_LIBRARIES})

#
# Set VS properties
#

if (MSVC)

	set_target_properties(
		UnitTests
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

message (STATUS "Copying DevIL runtime files...")

if (WIN32)
	file(COPY ${DEVIL_RUNTIME_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
	file(COPY ${DEVIL_RUNTIME_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
	file(COPY ${DEVIL_RUNTIME_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")
endif (WIN32)
