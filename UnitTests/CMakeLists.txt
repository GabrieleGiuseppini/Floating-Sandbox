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
	AABBTests.cpp
	AlgorithmsTests.cpp
	BoundedVectorTests.cpp
	BufferTests.cpp
	Buffer2DTests.cpp
	CircularListTests.cpp
	EnumFlagsTests.cpp
	FixedSizeVectorTests.cpp
	FloatingPointTests.cpp
	GameEventDispatcherTests.cpp
	GameGeometryTests.cpp
	GameMathTests.cpp
	LayoutHelperTests.cpp
	main.cpp
	Matrix2Tests.cpp
	MemoryStreamsTests.cpp
	ParameterSmootherTests.cpp
	PrecalculatedFunctionTests.cpp
	SettingsTests.cpp
	ShaderManagerTests.cpp
	ShipPreviewDirectoryManagerTests.cpp
	SliderCoreTests.cpp
	StrongTypeDefTests.cpp
	SysSpecificsTests.cpp
	TaskThreadTests.cpp
	TaskThreadPoolTests.cpp
	TemporallyCoherentPriorityQueueTests.cpp
	TextureAtlasTests.cpp
	TruncatedPriorityQueueTests.cpp
	TupleKeysTests.cpp
	UniqueBufferTests.cpp
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
	UILib
	${OPENGL_LIBRARIES}
	gmock
	gtest
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
