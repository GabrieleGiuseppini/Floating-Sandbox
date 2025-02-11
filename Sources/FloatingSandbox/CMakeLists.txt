
#
# FloatingSandbox application
#

set  (FLOATING_SANDBOX_SOURCES
	AudioController.h
	BootSettings.cpp
	BootSettings.h
	GLCanvas.h
	#MainApp.cpp
	#MainFrame.cpp
	#MainFrame.h
	Music.cpp
	Music.h
	MusicController.cpp
	MusicController.h
	SettingsManager.cpp
	SettingsManager.h
	SoundController.cpp
	SoundController.h
	Sounds.cpp
	Sounds.h
	ToolController.cpp
	ToolController.h
	Tools.cpp
	Tools.h
	UIPreferencesManager.cpp
	UIPreferencesManager.h
	UnhandledExceptionHandler.cpp
	UnhandledExceptionHandler.h
	UpdateChecker.cpp
	UpdateChecker.h
)

set  (UI_SOURCES
	UI/EventTickerPanel.cpp
	UI/EventTickerPanel.h

	UI/AboutDialog.cpp
	UI/AboutDialog.h
	UI/BootSettingsDialog.cpp
	UI/BootSettingsDialog.h
	UI/CheckForUpdatesDialog.cpp
	UI/CheckForUpdatesDialog.h
	UI/CreditsPanel.cpp
	UI/CreditsPanel.h
	UI/DebugDialog.cpp
	UI/DebugDialog.h
	UI/HelpDialog.cpp
	UI/HelpDialog.h
	UI/NewVersionDisplayDialog.cpp
	UI/NewVersionDisplayDialog.h
	UI/PreferencesDialog.cpp
	UI/PreferencesDialog.h
	UI/ProbePanel.cpp
	UI/ProbePanel.h
	UI/SettingsDialog.cpp
	UI/SettingsDialog.h
	UI/SplashScreenDialog.cpp
	UI/SplashScreenDialog.h
	UI/StartupTipDialog.cpp
	UI/StartupTipDialog.h
	UI/SwitchboardPanel.cpp
	UI/SwitchboardPanel.h
)

set(RESOURCE_FILES "")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	list(APPEND RESOURCE_FILES
		Resources/FloatingSandbox.rc
		Resources/ShipAAA.ico
		Resources/ShipBBB.ico)
else()
	list(APPEND RESOURCE_FILES
		Resources/ShipAAA.xpm
		Resources/ShipBBB.xpm)
endif()

source_group(" " FILES ${FLOATING_SANDBOX_SOURCES})
source_group("UI" FILES ${UI_SOURCES})
source_group("Resources" FILES ${RESOURCE_FILES})

add_executable (FloatingSandbox ${FLOATING_SANDBOX_SOURCES} ${UI_SOURCES} ${RESOURCE_FILES})

include(${wxWidgets_USE_FILE})

target_include_directories(FloatingSandbox PRIVATE .)
target_include_directories(FloatingSandbox PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(FloatingSandbox PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (FloatingSandbox PRIVATE
	Core
	Game
	OpenGLCore
	ShipBuilderLib
	Simulation
	UILib
	sfml-audio
	sfml-network
	sfml-system
	${OPENGL_LIBRARIES}
	${wxWidgets_LIBRARIES}
	${JPEG_LIBRARY}
	${PNG_LIBRARY}
	${ZLIB_LIBRARY}
	${ADDITIONAL_LIBRARIES})


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set_target_properties(FloatingSandbox PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:MSVCRTD")
endif ()


#
# VS properties
#

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	
	set_target_properties(
		FloatingSandbox
		PROPERTIES
			# Set debugger working directory to binary output directory
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

			# Set output directory to binary output directory - VS will add the configuration type
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

endif()



#
# Copy files
#

message (STATUS "Copying data files and runtime files for FloatingSandbox...")

file(COPY "${CMAKE_SOURCE_DIR}/Data" "${CMAKE_SOURCE_DIR}/Ships"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
file(COPY "${CMAKE_SOURCE_DIR}/Data" "${CMAKE_SOURCE_DIR}/Ships"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
file(COPY "${CMAKE_SOURCE_DIR}/Data" "${CMAKE_SOURCE_DIR}/Ships"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")

if (WIN32)
	# SFML runtime files
	file(COPY ${SFML_RUNTIME_DEBUG_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
	file(COPY ${SFML_RUNTIME_RELEASE_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
	file(COPY ${SFML_RUNTIME_RELEASE_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")
endif (WIN32)


####################################################
# Install
####################################################

# Main binary
install(TARGETS FloatingSandbox
	CONFIGURATIONS Release RelWithDebInfo Debug
        RUNTIME
	DESTINATION .)

# PDB
#TODO: disabled as for now we don't generate PDBs at Release
#if (MSVC)
#	install(FILES $<TARGET_PDB_FILE:FloatingSandbox>
#		CONFIGURATIONS Release
#		DESTINATION .)
#endif (MSVC)

# Runtime libraries
if (WIN32)
	# SFML
	install(FILES ${SFML_RUNTIME_RELEASE_LIBRARIES}
		DESTINATION .)
endif (WIN32)

# Resources
install(DIRECTORY "${CMAKE_SOURCE_DIR}/Data"
	DESTINATION .)

# Guides
install(DIRECTORY "${CMAKE_SOURCE_DIR}/Guides"
	DESTINATION .)

# Ships
install(DIRECTORY "${CMAKE_SOURCE_DIR}/Ships"
	DESTINATION .
	PATTERN "default_ship.png" EXCLUDE)
install(FILES "${CMAKE_SOURCE_DIR}/Ships/R.M.S. Titanic (With Power).shp2"
	DESTINATION Ships
	RENAME "default_ship.shp2")

# License et al
install(FILES "${CMAKE_SOURCE_DIR}/license.txt" "${CMAKE_SOURCE_DIR}/changes.txt" "${CMAKE_SOURCE_DIR}/README.md"
	DESTINATION .)
