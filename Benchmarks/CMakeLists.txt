set (BENCHMARK_SOURCES
        DiffuseLight.cpp
        DivisionByZero.cpp
        GameMath.cpp
        Logarithm.cpp
        PrecalculatedFunction.cpp
        SingleVectorNormalization.cpp
        TopN.cpp
        UpdateSpringForces.cpp
        Utils.cpp
        Utils.h
        VectorNormalization.cpp
)

source_group(" " FILES ${BENCHMARK_SOURCES})

add_executable (Benchmarks ${BENCHMARK_SOURCES})

target_link_libraries (Benchmarks
        GameCoreLib
        GameLib
        GPUCalcLib
        ${OPENGL_LIBRARIES}
        benchmark::benchmark
        benchmark::benchmark_main
        ${ADDITIONAL_LIBRARIES})


#
# Set VS properties
#

if (MSVC)
        
        set_target_properties(
                Benchmarks
                PROPERTIES
                        # Set debugger working directory to binary output directory
                        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

                        # Set output directory to binary output directory - VS will add the configuration type
                        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )

        #set_property(TARGET Benchmarks
        set_target_properties(
                Benchmarks
                PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_DEBUG TRUE)

endif (MSVC)


#
# Copy files
#

message (STATUS "Copying DevIL runtime files...")

if (WIN32)
        #file(COPY ${DEVIL_RUNTIME_LIBRARIES}
        #        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
        file(COPY ${DEVIL_RUNTIME_LIBRARIES}
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
        file(COPY ${DEVIL_RUNTIME_LIBRARIES}
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")
endif (WIN32)
