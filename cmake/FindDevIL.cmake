find_path(IL_INCLUDE_DIR IL/il.h
  HINTS ${DevIL_ROOT_DIR}/include
  DOC "The path to the directory that contains il.h"
)

find_library(IL_LIBRARIES_RELEASE
  NAMES IL DEVIL
  HINTS ${DevIL_ROOT_DIR} ${DevIL_ROOT_DIR}/lib/ ${DevIL_ROOT_DIR}/Release/lib/ ${DevIL_LIB_DIR}
  PATH_SUFFIXES libx32 lib64 lib lib32 x86 x64
  DOC "The file that corresponds to the base il library."
)

find_library(ILU_LIBRARIES_RELEASE
  NAMES ILU
  HINTS ${DevIL_ROOT_DIR} ${DevIL_ROOT_DIR}/lib/ ${DevIL_ROOT_DIR}/Release/lib/ ${DevIL_LIB_DIR}
  PATH_SUFFIXES libx32 lib64 lib lib32 x86 x64
  DOC "The file that corresponds to the il utility library."
)

set(IL_LIBRARIES ${IL_LIBRARIES_RELEASE})
set(ILU_LIBRARIES ${ILU_LIBRARIES_RELEASE})

if(WIN32)
	if(FS_USE_STATIC_LIBS)

		# If we're using static linking, then we assume DevIL is also statically linked,
		# hence we want different debug/release libs

		find_library(IL_LIBRARIES_DEBUG
		  NAMES IL DEVIL
		  HINTS ${DevIL_ROOT_DIR} ${DevIL_ROOT_DIR}/lib/ ${DevIL_ROOT_DIR}/Debug/lib/ ${DevIL_LIB_DIR}
		  PATH_SUFFIXES libx32 lib64 lib lib32 x86 x64
		  DOC "The file that corresponds to the base il library."
		)

		find_library(ILU_LIBRARIES_DEBUG
		  NAMES ILU
		  HINTS ${DevIL_ROOT_DIR} ${DevIL_ROOT_DIR}/lib/ ${DevIL_ROOT_DIR}/Debug/lib/ ${DevIL_LIB_DIR}
		  PATH_SUFFIXES libx32 lib64 lib lib32 x86 x64
		  DOC "The file that corresponds to the il utility library."
		)

		set(IL_LIBRARIES optimized ${IL_LIBRARIES_RELEASE} debug ${IL_LIBRARIES_DEBUG})
		set(ILU_LIBRARIES optimized ${ILU_LIBRARIES_RELEASE} debug ${ILU_LIBRARIES_DEBUG})
	endif()
endif()


include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(DevIL DEFAULT_MSG
                                  IL_LIBRARIES
                                  ILU_LIBRARIES
                                  IL_INCLUDE_DIR)

# provide legacy variable for compatiblity
set(IL_FOUND ${DevIL_FOUND})