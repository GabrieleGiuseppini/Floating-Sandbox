cmake_minimum_required (VERSION 2.6)

if (POLICY CMP0048)
	cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project (FloatingSandbox)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(MSVC_USE_STATIC_LINKING "Force static linking on MSVC" OFF)

####################################################
# Custom CMake modules
####################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

####################################################
# User settings file
####################################################

set(USER_SETTINGS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/UserSettings.cmake")
if(EXISTS "${USER_SETTINGS_FILE}")
	message(STATUS "Loading user specific settings from ${USER_SETTINGS_FILE}")
	include("${USER_SETTINGS_FILE}")
endif()

####################################################
#  External library configuration
####################################################

list(APPEND CMAKE_PREFIX_PATH "${BENCHMARK_ROOT_DIR}")

find_package(PicoJSON REQUIRED)
find_package(benchmark REQUIRED)

find_package(wxWidgets REQUIRED base gl core html media)

find_package(DevIL REQUIRED)
if (WIN32)
	get_filename_component(DEVIL_LIB_DIR ${IL_LIBRARIES} DIRECTORY)
	file(GLOB DEVIL_RUNTIME_LIBRARIES
		${DEVIL_LIB_DIR}/*.dll)
endif()

find_package(SFML 2.5 COMPONENTS system audio REQUIRED)
if (WIN32)
	find_path(SFML_BIN_DIR
		NAMES sfml-system-2.dll openal32.dll
		HINTS ${SFML_DIR}/../../../
		PATH_SUFFIXES SFML bin
	)

	if(NOT EXISTS "${SFML_BIN_DIR}")
		message(FATAL_ERROR "Could not find SFML binary directory")
	endif()

	# Pointing to System32 would result in copying all dlls in that directory
	if(WIN32 AND SFML_BIN_DIR MATCHES "System32")
		message(FATAL_ERROR "SFML_BIN_DIR ('${SFML_BIN_DIR}') seems to be pointing to System32 folder, which is not allowed")
	endif()

	set(SFML_RUNTIME_RELEASE_LIBRARIES 
		${SFML_BIN_DIR}/sfml-audio-2.dll
		${SFML_BIN_DIR}/sfml-network-2.dll
		${SFML_BIN_DIR}/sfml-system-2.dll
		${SFML_BIN_DIR}/openal32.dll)

	set(SFML_RUNTIME_DEBUG_LIBRARIES 
		${SFML_BIN_DIR}/sfml-audio-d-2.dll
		${SFML_BIN_DIR}/sfml-network-d-2.dll
		${SFML_BIN_DIR}/sfml-system-d-2.dll
		${SFML_BIN_DIR}/openal32.dll)
endif()

find_package(OpenGL REQUIRED)

if(MSVC_USE_STATIC_LINKING) # No way to 'negate' a variable easily
	set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
else()
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(${GTEST_DIR} gtest EXCLUDE_FROM_ALL)

####################################################
# Flags
####################################################

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")

	# Make sure the CRT is statically linked, and that we use W4 instead of W3
	foreach(flag_var
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
		if(MSVC_USE_STATIC_LINKING)
			if(${flag_var} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
			endif(${flag_var} MATCHES "/MD")
		endif()
		if(${flag_var} MATCHES "/W3")
			string(REGEX REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/W3")
		set(${flag_var} "${${flag_var}} /W4")
	endforeach(flag_var)

	# Make RelWithDebInfo use aggressive inlining
	#if(${CMAKE_CXX_FLAGS_RELWITHDEBINFO} MATCHES "/Ob1")
	#	string(REGEX REPLACE "/Ob1" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	#endif(${CMAKE_CXX_FLAGS_RELWITHDEBINFO} MATCHES "/Ob1")
	#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2")

	# Disable incremental linking in Release
	foreach(flag_var
		CMAKE_EXE_LINKER_FLAGS_RELEASE)
		if(${flag_var} MATCHES "/INCREMENTAL")
			string(REGEX REPLACE "/INCREMENTAL:NO" "" ${flag_var} "${${flag_var}}")
			string(REGEX REPLACE "/INCREMENTAL" "" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/INCREMENTAL")
		set(${flag_var} "${${flag_var}} /INCREMENTAL:NO")
	endforeach(flag_var)

	# Fine-tune optimizations
	string(REGEX REPLACE "/O2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast /Ox /GS-")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /fp:fast /Ox /GS-")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:strict /DFLOATING_POINT_CHECKS")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT;MSVCRTD")

	add_compile_options(/permissive-)

elseif(CMAKE_COMPILER_IS_GNUXX)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W4 -O3 -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /W4 -O3 -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:strict -DFLOATING_POINT_CHECKS")
endif()

add_definitions(-DPICOJSON_USE_INT64)

message (STATUS "cxx Flags:" ${CMAKE_CXX_FLAGS})
message (STATUS "cxx Flags Release:" ${CMAKE_CXX_FLAGS_RELEASE})
message (STATUS "cxx Flags RelWithDebInfo:" ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message (STATUS "cxx Flags Debug:" ${CMAKE_CXX_FLAGS_DEBUG})
message (STATUS "c Flags:" ${CMAKE_C_FLAGS})
message (STATUS "c Flags Release:" ${CMAKE_C_FLAGS_RELEASE})
message (STATUS "c Flags RelWithDebInfo:" ${CMAKE_C_FLAGS_RELWITHDEBINFO})
message (STATUS "c Flags Debug:" ${CMAKE_C_FLAGS_DEBUG})
message (STATUS "exe Linker Flags Release:" ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
message (STATUS "exe Linker Flags RelWithDebInfo:" ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
message (STATUS "exe Linker Flags Debug:" ${CMAKE_EXE_LINKER_FLAGS_DEBUG})

####################################################
# Misc
####################################################

if (MSVC)
	set(ADDITIONAL_LIBRARIES "comctl32;rpcrt4;advapi32") # winmm.lib wsock32.lib
else(MSVC)
	set(ADDITIONAL_LIBRARIES "")
endif(MSVC)



####################################################
# Sub-projects
####################################################

add_subdirectory(Benchmarks)
add_subdirectory(FloatingSandbox)
add_subdirectory(Game)
add_subdirectory(GameCore)
add_subdirectory(GameOpenGL)
add_subdirectory(GPUCalc)
add_subdirectory(GPUCalcTest)
add_subdirectory(ShipTools)
add_subdirectory(UIControls)
add_subdirectory(UnitTests)



####################################################
# Install
####################################################

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/Install" CACHE PATH "..." FORCE)
