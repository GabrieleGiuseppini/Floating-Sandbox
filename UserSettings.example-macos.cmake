# Example UserSettings.cmake for macOS (Homebrew-based).
# Copy this to UserSettings.cmake and adjust the paths if needed.
# See BUILD-macOS.md for full instructions.

# Homebrew-provided libraries are shared, so we link dynamically.
set(FS_USE_STATIC_LIBS OFF)

# Homebrew prefix: /opt/homebrew on Apple Silicon, /usr/local on Intel Macs.
set(HOMEBREW_PREFIX "/opt/homebrew")

# Source-only dependencies (see BUILD-macOS.md). Change if cloned elsewhere.
set(REPOS_ROOT "$ENV{HOME}/git")
set(PICOJSON_DIR "${REPOS_ROOT}/picojson")
set(GTEST_DIR "${REPOS_ROOT}/googletest")

# wxWidgets (Homebrew)
set(wxWidgets_ROOT "${HOMEBREW_PREFIX}/opt/wxwidgets")
set(wxWidgets_CONFIG_EXECUTABLE "${HOMEBREW_PREFIX}/opt/wxwidgets/bin/wx-config")
set(wxWidgets_USE_STATIC OFF)

# SFML 2 (Homebrew 'sfml@2' formula - keg-only, hence the explicit paths)
set(SFML_DIR "${HOMEBREW_PREFIX}/opt/sfml@2/lib/cmake/SFML")
set(SFML_ROOT "${HOMEBREW_PREFIX}/opt/sfml@2")
link_directories("${HOMEBREW_PREFIX}/opt/sfml@2/lib")

# Define macro that creates post-install actions
macro(DefineUserPostInstall)
endmacro()
