set (BENCHMARK_SOURCES
	AutoTexturization.cpp
        DiffuseLight.cpp
        DivisionByZero.cpp
        GameMath.cpp
        Logarithm.cpp
	MakeAABBWeightedUnion.cpp
        PrecalculatedFunction.cpp
        SingleVectorNormalization.cpp
	Step.cpp
        TopN.cpp
        UpdateSpringForces.cpp
        Utils.cpp
        Utils.h
        VectorNormalization.cpp
)

source_group(" " FILES ${BENCHMARK_SOURCES})

add_executable (Benchmarks ${BENCHMARK_SOURCES})

target_include_directories(Benchmarks PRIVATE .)
target_link_libraries (Benchmarks
	Core
        Game
	Simulation
        #GPUCalcLib        
        benchmark::benchmark
        benchmark::benchmark_main
	${OPENGL_LIBRARIES}
        ${ADDITIONAL_LIBRARIES})


#
# Set VS properties
#

if (MSVC)

	# Google benchmark has its own warnings, can't do anything about them
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX-")
        
        set_target_properties(
                Benchmarks
                PROPERTIES
                        # Set debugger working directory to binary output directory
                        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

                        # Set output directory to binary output directory - VS will add the configuration type
                        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )

        set_target_properties(
                Benchmarks
                PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_DEBUG TRUE)

endif (MSVC)


#
# Copy files
#

message (STATUS "Copying data files for Benchmarks...")

file(COPY "${CMAKE_SOURCE_DIR}/Data"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
