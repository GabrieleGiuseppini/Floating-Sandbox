
add_subdirectory(Core)
add_subdirectory(Game)
add_subdirectory(OpenGLCore)
add_subdirectory(Render)
add_subdirectory(Simulation)
add_subdirectory(ShipBuilderLib)
add_subdirectory(ShipTools)
add_subdirectory(UILib)
add_subdirectory(UnitTests)

# TODO: others

if(FS_BUILD_BENCHMARKS)
	add_subdirectory(Benchmarks)
endif()
