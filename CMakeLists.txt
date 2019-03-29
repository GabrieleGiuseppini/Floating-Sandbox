cmake_minimum_required (VERSION 2.6)

project (FloatingSandbox)
 
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

####################################################
# Hardcoded paths
####################################################

set(SDK_ROOT "C:/Users/Neurodancer/source/SDK" CACHE PATH "..." FORCE)
set(REPOS_ROOT "C:/Users/Neurodancer/source/repos" CACHE PATH "..." FORCE)


####################################################
#                External libraries 
#
# The library paths I'm using are from my MSVC build
# environment; just replace my paths with yours.
# For platforms with no difference between Debug and
# Release builds, just use the same library for both.
####################################################

list(APPEND CMAKE_PREFIX_PATH "${SDK_ROOT}/PicoJSON")
list(APPEND CMAKE_PREFIX_PATH "${SDK_ROOT}/benchmark")
list(APPEND CMAKE_PREFIX_PATH "${SDK_ROOT}/libsimdpp-2.1")
list(APPEND CMAKE_PREFIX_PATH "${SDK_ROOT}/DevIL")
list(APPEND CMAKE_PREFIX_PATH "${SDK_ROOT}/SFML")
list(APPEND CMAKE_LIBRARY_PATH "${SDK_ROOT}/DevIL")
set(SFML_ROOT "${SDK_ROOT}/SFML")
set(wxWidgets_ROOT_DIR "${SDK_ROOT}/wxWidgets")
set(GOOGLE_TEST_ROOT "${REPOS_ROOT}/googletest")

find_path(PICOJSON_INCLUDE_DIRS
    NAMES picojson/picojson.h)

find_package(benchmark REQUIRED) 

find_path(LIBSIMDPP_INCLUDE_DIRS
    NAMES simdpp/simd.h)

find_package(wxWidgets REQUIRED base gl core html media)

find_package(DevIL REQUIRED)
if (WIN32)
	find_path(DEVIL_LIB_DIR
		NAMES DevIL.lib
		PATH_SUFFIXES DevIL libx32 lib64 lib lib32)
	file(GLOB DEVIL_RUNTIME_LIBRARIES 
		${DEVIL_LIB_DIR}/*.dll)
endif()

find_package(SFML COMPONENTS system audio REQUIRED)
if (WIN32)
	find_path(SFML_BIN_DIR
		NAMES sfml-system-2.dll openal32.dll
		PATH_SUFFIXES SFML bin)

	file(GLOB SFML_RUNTIME_RELEASE_LIBRARIES 
		${SFML_BIN_DIR}/*.dll)
	list(FILTER SFML_RUNTIME_RELEASE_LIBRARIES EXCLUDE REGEX "-d-") 

	file(GLOB SFML_RUNTIME_DEBUG_LIBRARIES 
		${SFML_BIN_DIR}/*.dll)
	list(FILTER SFML_RUNTIME_DEBUG_LIBRARIES INCLUDE REGEX "-d-") 
	list(APPEND SFML_RUNTIME_DEBUG_LIBRARIES ${SFML_BIN_DIR}/openal32.dll) 

endif()


find_package(OpenGL REQUIRED)


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
		if(${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/MD")
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
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl")
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
add_subdirectory(UnitTests)



####################################################
# Install
####################################################

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/Install" CACHE PATH "..." FORCE)
