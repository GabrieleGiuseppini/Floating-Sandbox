# Enable static linking
set(FS_USE_STATIC_LIBS ON)

# Define dependency locations
set(SDK_ROOT "C:/Users/Neurodancer/source/SDK")
set(REPOS_ROOT "C:/Users/Neurodancer/source/repos")

set(DevIL_ROOT_DIR "${SDK_ROOT}/DevIL")
set(DevIL_LIB_DIR "${SDK_ROOT}/DevIL/lib/x64/Release/")
set(wxWidgets_ROOT_DIR "${SDK_ROOT}/wxWidgets")
set(SFML_DIR "${SDK_ROOT}/SFML")
set(GTEST_DIR "${REPOS_ROOT}/googletest")
set(PICOJSON_DIR "${SDK_ROOT}/PicoJSON")
set(BENCHMARK_ROOT_DIR "${SDK_ROOT}/benchmark")

# Define macro that creates post-install actions
macro(DefineUserPostInstall)
  message(STATUS "No user-defined post-install actions")
endmacro()