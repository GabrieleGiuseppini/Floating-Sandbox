
#
# ShipBuilder standalone application
#

set  (SHIPBUILDER_SOURCES
	MainApp.cpp
)

source_group(" " FILES ${SHIPBUILDER_SOURCES})

set(RESOURCE_FILES "")
if(MSVC)
	list(APPEND RESOURCE_FILES
		Resources/ShipBuilder.rc
		Resources/ShipAAA.ico
		Resources/ShipBBB.ico)
else()
	list(APPEND RESOURCE_FILES
		Resources/ShipAAA.xpm
		Resources/ShipBBB.xpm)
endif()

source_group("Resources" FILES ${RESOURCE_FILES})

add_executable (ShipBuilder ${SHIPBUILDER_SOURCES} ${RESOURCE_FILES})

include(${wxWidgets_USE_FILE})

target_include_directories(ShipBuilder PRIVATE .)
target_include_directories(ShipBuilder PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(ShipBuilder PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (ShipBuilder
	GameCoreLib
	GameLib
	GameOpenGLLib
	ShipBuilderLib
	UILib
	${OPENGL_LIBRARIES}
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})


if (MSVC)
	set_target_properties(ShipBuilder PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:MSVCRTD")
endif (MSVC)


#
# VS properties
#

if(MSVC)
	
	set_target_properties(
		ShipBuilder
		PROPERTIES
			# Set debugger working directory to binary output directory
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

			# Set output directory to binary output directory - VS will add the configuration type
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

endif()

#
# Copy files
#

message (STATUS "Copying data files for ShipBuilder...")

file(COPY "${CMAKE_SOURCE_DIR}/Data"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
file(COPY "${CMAKE_SOURCE_DIR}/Data"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
file(COPY "${CMAKE_SOURCE_DIR}/Data"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")
