
#
# ShipTools application
#

set  (SHIP_TOOLS_SOURCES
	Baker.h
	Helpers.cpp
	Helpers.h
	Main.cpp
	Quantizer.cpp
	Quantizer.h
	Resizer.cpp
	Resizer.h
	ShipAnalyzer.cpp
	ShipAnalyzer.h
	)

source_group(" " FILES ${SHIP_TOOLS_SOURCES})

add_executable (ShipTools ${SHIP_TOOLS_SOURCES})

target_include_directories(ShipTools PRIVATE ${IL_INCLUDE_DIR})

target_link_libraries (ShipTools
	GameCoreLib
	GameLib
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ADDITIONAL_LIBRARIES})


if (MSVC)
	set_target_properties(ShipTools PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE /NODEFAULTLIB:MSVCRTD")
else (MSVC)
endif (MSVC)


#
# Set VS properties
#

if (MSVC)

	set_target_properties(
		ShipTools
		PROPERTIES
			# Set debugger working directory to binary output directory
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

			# Set output directory to binary output directory - VS will add the configuration type
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)

endif (MSVC)
