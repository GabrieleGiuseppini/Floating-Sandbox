# Enable static linking
set(FS_USE_STATIC_LIBS ON)

# Define dependency locations
set(SDK_ROOT "C:/Users/Neurodancer/source/SDK")
set(REPOS_ROOT "C:/Users/Neurodancer/source/repos")

set(ZLIB_ROOT "${SDK_ROOT}/ZLib-1.2.11")
set(JPEG_ROOT "${SDK_ROOT}/jpeg-9d")
set(PNG_ROOT "${SDK_ROOT}/LibPNG-1.6.37")
set(wxWidgets_ROOT "${SDK_ROOT}/wxWidgets-3.1.4")
set(SFML_ROOT "${SDK_ROOT}/SFML-2.5.1/lib/cmake/SFML")
set(GTEST_DIR "${REPOS_ROOT}/googletest")
set(PICOJSON_DIR "${SDK_ROOT}/PicoJSON")

# Define macro that creates post-install actions
macro(DefineUserPostInstall)
  message(STATUS "No user-defined post-install actions")
endmacro()