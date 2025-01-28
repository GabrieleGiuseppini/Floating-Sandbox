enable_testing()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING /DGTEST_HAS_TR1_TUPLE=0")
endif()

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
	ColorsTests.cpp
	DeSerializationBufferTests.cpp
	#ElectricalPanelTests.cpp
	EndianTests.cpp
	EnumFlagsTests.cpp
	FileSystemTests.cpp
	FinalizerTests.cpp
	FixedSizeVectorTests.cpp
	FloatingPointTests.cpp
	#GameEventDispatcherTests.cpp
	GameGeometryTests.cpp
	GameMathTests.cpp
	ImageToolsTests.cpp
	IndexRemapTests.cpp
	#InstancedElectricalElementSetTests.cpp
	IntegralSystemTests.cpp
	#LayerTests.cpp
	#LayoutHelperTests.cpp
	main.cpp
	Matrix2Tests.cpp
	MemoryStreamsTests.cpp
	ParameterSmootherTests.cpp
	PortableTimepointTests.cpp
	PrecalculatedFunctionTests.cpp
	#RopeBufferTests.cpp
	#SettingsTests.cpp
	ShaderManagerTests.cpp
	#ShipDefinitionFormatDeSerializerTests.cpp
	#ShipNameNormalizerTests.cpp
	#ShipPreviewDirectoryManagerTests.cpp
	#ShipTests.cpp
	#SliderCoreTests.cpp
	StrongTypeDefTests.cpp
	SysSpecificsTests.cpp
	TaskThreadTests.cpp
	TemporallyCoherentPriorityQueueTests.cpp
	TestingUtils.cpp
	TestingUtils.h
	TextureAtlasTests.cpp
	TextureDatabaseTests.cpp
	ThreadPoolTests.cpp
	TruncatedPriorityQueueTests.cpp
	TupleKeysTests.cpp
	UniqueBufferTests.cpp
	UtilsTests.cpp
	VectorsTests.cpp
	#VersionTests.cpp
)

source_group(" " FILES ${UNIT_TEST_SOURCES})

add_executable (UnitTests ${UNIT_TEST_SOURCES})
add_test (UnitTests UnitTests)

target_link_libraries (UnitTests
	Core
	Game
	OpenGLCore
	Render
	#GPUCalcLib
	#ShipBuilderLib
	#UILib
	${OPENGL_LIBRARIES}
	gmock
	gtest
	${ADDITIONAL_LIBRARIES})

#
# Setup test
#

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_test(NAME RunUnitTests COMMAND $<TARGET_FILE:UnitTests>)
endif ()

#
# Set VS properties
#

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	set_target_properties(
		UnitTests
		PROPERTIES
			# Set debugger working directory to binary output directory
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

			# Set output directory to binary output directory - VS will add the configuration type
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)

endif ()
