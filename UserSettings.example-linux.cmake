# Enable static linking
set(FS_USE_STATIC_LIBS ON)

# Define dependency locations
set(SDK_ROOT "~/fs_libs")
set(REPOS_ROOT "~/git")

set(DevIL_ROOT_DIR "${SDK_ROOT}/DevIL")
set(SFML_DIR "${SDK_ROOT}/SFML/lib/cmake/SFML")
set(BENCHMARK_ROOT_DIR "${SDK_ROOT}/benchmark")
set(wxWidgets_ROOT_DIR "${SDK_ROOT}/wxWidgets")
set(GTEST_DIR "${REPOS_ROOT}/googletest")
set(PICOJSON_DIR "${REPOS_ROOT}/picojson")

# Define macro that creates post-install actions
macro(DefineUserPostInstall)
endmacro()