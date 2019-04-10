find_path(IL_INCLUDE_DIR IL/il.h
  HINTS ${DevIL_ROOT_DIR}/include
  DOC "The path to the directory that contains il.h"
)

find_library(IL_LIBRARIES
  NAMES IL DEVIL
  HINTS ${DevIL_ROOT_DIR} ${DevIL_ROOT_DIR}/lib/ ${DevIL_LIB_DIR}
  PATH_SUFFIXES libx32 lib64 lib lib32 x86 x64
  DOC "The file that corresponds to the base il library."
)

find_library(ILUT_LIBRARIES
  NAMES ILUT
  HINTS ${DevIL_ROOT_DIR} ${DevIL_ROOT_DIR}/lib/ ${DevIL_LIB_DIR}
  PATH_SUFFIXES libx32 lib64 lib lib32 x86 x64
  DOC "The file that corresponds to the il (system?) utility library."
)

find_library(ILU_LIBRARIES
  NAMES ILU
  HINTS ${DevIL_ROOT_DIR} ${DevIL_ROOT_DIR}/lib/ ${DevIL_LIB_DIR}
  PATH_SUFFIXES libx32 lib64 lib lib32 x86 x64
  DOC "The file that corresponds to the il utility library."
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DevIL DEFAULT_MSG
                                  IL_LIBRARIES
                                  ILU_LIBRARIES
                                  IL_INCLUDE_DIR)
# provide legacy variable for compatiblity
set(IL_FOUND ${DevIL_FOUND})