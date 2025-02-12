add_subdirectory(Core)
add_subdirectory(FloatingSandbox)
add_subdirectory(Game)
add_subdirectory(OpenGLCore)
add_subdirectory(Render)
add_subdirectory(Simulation)
add_subdirectory(ShipBuilder)
add_subdirectory(ShipBuilderLib)
add_subdirectory(ShipTools)
add_subdirectory(UILib)
add_subdirectory(UnitTests)

# These are disabled as they are inconsequential to Floating Sandbox
#add_subdirectory(GPUCalc)
#add_subdirectory(GPUCalcTest)

if(FS_BUILD_BENCHMARKS)
	add_subdirectory(Benchmarks)
endif()
